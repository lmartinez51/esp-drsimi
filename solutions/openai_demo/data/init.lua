-- =============================================================================
-- ESP-Camila Automation Engine
-- File: /littlefs/init.lua  (flashed from data/init.lua)
--
-- Architecture:
--   - ir_db  : RAM table for IR device/button codes. Persisted to ir_db.json.
--   - rules_db: RAM table for automation rules. Persisted to rules.json via C.
--   - register_rule(rule): IPC entry point called by the C worker on every
--     incoming esp_claw_rule_t message. Routes by trigger string.
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. IR Database Initialization
-- -----------------------------------------------------------------------------
ir_db = ir_db or {}
ir_learning_target_device = nil
ir_learning_target_button  = nil

-- Stateful two-pass line-by-line deserializer.
-- Replaces the brittle {[^}]+} block-capture regex that failed on multi-button
-- devices and was vulnerable to scientific notation and } in device names.
local function load_ir_db()
    local content = nil
    if ir.read_db then
        content = ir.read_db()
    end

    if not content or content == "" then
        print("ir_db: file not found or empty. Initializing clean DB.")
        ir_db = {}
        return
    end

    ir_db = {}
    local current_device = nil

    -- Append a sentinel newline so the last line is always captured.
    for line in string.gmatch(content .. "\n", "([^\n]*)\n") do

        -- Pattern 1: Device header line  →  "TV Sala": {
        -- The line must end with { (after optional whitespace).
        -- Device names containing } are safely ignored because the }
        -- is inside the quoted string, not at the start of the line.
        local dev = line:match('^%s*"([^"]+)"%s*:%s*{%s*$')
        if dev then
            current_device = dev
            ir_db[current_device] = {}

        else
            -- Pattern 2: Button entry  →  "power": "0x00FF00FF"
            -- Only fires when inside a device block (current_device ~= nil).
            local btn, hex = line:match('^%s*"([^"]+)"%s*:%s*"0x(%x+)"%s*,?%s*$')
            if btn and hex and current_device then
                -- Canonicalize to uppercase 0x prefix for consistent comparison.
                ir_db[current_device][btn] = "0x" .. string.upper(hex)
            end

            -- Pattern 3: Closing brace  →  }  or  },
            -- Resets the device context. Only matches a line that is solely a
            -- closing brace (with optional comma and whitespace), so it cannot
            -- be confused with a device name that happens to contain }.
            if line:match('^%s*}%s*,?%s*$') then
                current_device = nil
            end
        end
    end

    print("ir_db: loaded successfully.")
end

load_ir_db()

-- -----------------------------------------------------------------------------
-- 2. IR Database Save (Serializer)
-- -----------------------------------------------------------------------------
-- All codes in ir_db are stored as canonical "0x%08X" strings.
-- The serializer wraps them in JSON quotes directly — no tostring() conversion,
-- no risk of scientific notation corruption.
function ir.save_db()
    if not ir.write_db then
        print("ERROR: ir.write_db C-binding not found.")
        return
    end

    local chunks = {"{\n"}
    local first_dev = true
    for dev, buttons in pairs(ir_db) do
        if not first_dev then table.insert(chunks, ",\n") end
        first_dev = false
        table.insert(chunks, '  "' .. dev .. '": {\n')
        
        local first_btn = true
        for btn, code in pairs(buttons) do
            if not first_btn then table.insert(chunks, ",\n") end
            first_btn = false
            table.insert(chunks, '    "' .. btn .. '": "' .. code .. '"')
        end
        table.insert(chunks, "\n  }")
    end
    table.insert(chunks, "\n}\n")
    
    local json_str = table.concat(chunks)

    if ir.write_db(json_str) then
        print("ir_db: flushed to LittleFS.")
    else
        print("ERROR: ir.write_db failed to write ir_db.json.")
    end
end

-- -----------------------------------------------------------------------------
-- 2b. Flexible Token & Synonym IR Device Normalization & Matcher
-- -----------------------------------------------------------------------------
local function normalize_text(str)
    if not str or str == "" then return "" end

    -- Convert to lowercase
    str = string.lower(str)

    -- Separate CamelCase (e.g. "tvRecamara" -> "tv Recamara")
    str = str:gsub("(%l)(%u)", "%1 %2")
    str = string.lower(str)

    -- Strip diacritics / accents in Spanish UTF-8
    local accents = {
        ["á"] = "a", ["é"] = "e", ["í"] = "i", ["ó"] = "o", ["ú"] = "u",
        ["ü"] = "u", ["ñ"] = "n",
        ["Á"] = "a", ["É"] = "e", ["Í"] = "i", ["Ó"] = "o", ["Ú"] = "u",
        ["Ü"] = "u", ["Ñ"] = "n"
    }
    for k, v in pairs(accents) do
        str = str:gsub(k, v)
    end

    -- Replace punctuation and underscores with spaces
    str = str:gsub("[%c%p]", " ")

    -- Tokenize and apply synonyms & stop words
    local tokens = {}
    local stop_words = {
        ["de"] = true, ["la"] = true, ["el"] = true, ["del"] = true,
        ["los"] = true, ["las"] = true, ["un"] = true, ["una"] = true
    }
    local synonyms = {
        ["television"] = "tv",
        ["tele"] = "tv",
        ["cuarto"] = "recamara",
        ["habitacion"] = "recamara",
        ["dormitorio"] = "recamara"
    }

    for word in str:gmatch("%S+") do
        local norm_w = synonyms[word] or word
        if not stop_words[norm_w] then
            table.insert(tokens, norm_w)
        end
    end

    return table.concat(tokens, " ")
end

local function find_ir_device(query_target)
    if not ir_db or not query_target or query_target == "" then
        return nil, nil
    end

    -- Fast-path: Direct key lookup
    if ir_db[query_target] then
        return query_target, ir_db[query_target]
    end

    local norm_query = normalize_text(query_target)
    if norm_query == "" then
        return nil, nil
    end

    -- Extract query tokens
    local query_tokens = {}
    local query_token_count = 0
    for token in norm_query:gmatch("%S+") do
        query_tokens[token] = true
        query_token_count = query_token_count + 1
    end

    local best_match_dev = nil
    local best_match_buttons = nil
    local best_score = 0

    for dev_name, buttons in pairs(ir_db) do
        local norm_key = normalize_text(dev_name)

        -- 1. Direct Normalized Match
        if norm_key == norm_query then
            return dev_name, buttons
        end

        -- 2. Substring / Containment Match
        if #norm_key > 0 and #norm_query > 0 and
           (string.find(norm_key, norm_query, 1, true) or string.find(norm_query, norm_key, 1, true)) then
            if best_score < 2 then
                best_score = 2
                best_match_dev = dev_name
                best_match_buttons = buttons
            end
        end

        -- 3. Token Overlap Match
        if best_score < 2 then
            local key_tokens = {}
            local matched_count = 0
            local key_token_count = 0
            for token in norm_key:gmatch("%S+") do
                key_tokens[token] = true
                key_token_count = key_token_count + 1
                if query_tokens[token] then
                    matched_count = matched_count + 1
                end
            end

            if query_token_count > 0 and (matched_count == query_token_count or matched_count == key_token_count) then
                if best_score < 1 then
                    best_score = 1
                    best_match_dev = dev_name
                    best_match_buttons = buttons
                end
            end
        end
    end

    if best_match_dev then
        print("[IR_LOOKUP] Coincidencia difusa en ir_db para '" .. query_target .. "' -> '" .. best_match_dev .. "'")
        return best_match_dev, best_match_buttons
    end

    return nil, nil
end

-- -----------------------------------------------------------------------------
-- 3. Rules DB Initialization
-- -----------------------------------------------------------------------------
rules_db = rules_db or {}

-- -----------------------------------------------------------------------------
-- 4. Rules DB Sanitization (one-time boot cleanup)
-- -----------------------------------------------------------------------------
-- Removes phantom IR entries that were incorrectly written into rules.json
-- during the conflation period (when the broken routing fell through to the
-- standard rule-creation path). Runs once at boot before any tool call arrives.
local function sanitize_rules_db()
    if type(rules_db) ~= "table" then return end

    local ir_phantom_keys = {
        -- IPC trigger names written during the conflation period
        ["LUA_TOOL_IR_LEARN"]       = true,
        ["LUA_TOOL_IR_TRANSMIT"]    = true,
        ["LUA_TOOL_IR_SAVE"]        = true,
        ["LUA_TOOL_IR_GET_DEVICES"] = true,
        ["LUA_TOOL_IR_DELETE"]      = true,
        ["LUA_CMD_IR_LEARNED"]      = true,
        ["LUA_CMD_IR_TIMEOUT"]      = true,
        -- Raw WebRTC JSON tool names that fell through the broken minimal_lua stub
        ["ir_learn_button"]         = true,
        ["ir_transmit_command"]     = true,
        ["ir_get_devices"]          = true,
        ["ir_save_database"]        = true,
        ["ir_delete_device"]        = true,
    }

    local removed = 0
    for key in pairs(rules_db) do
        if ir_phantom_keys[key] then
            rules_db[key] = nil
            removed = removed + 1
            print("SANITIZE: removed phantom IR rule key: " .. key)
        end
    end

    if removed > 0 then
        if c_save_rules then
            c_save_rules()
            print("SANITIZE: rules.json flushed. " .. removed .. " phantom(s) removed.")
        end
    else
        print("SANITIZE: rules.json is clean.")
    end
end

sanitize_rules_db()

-- -----------------------------------------------------------------------------
-- 5. IPC Entry Point: register_rule(rule)
-- -----------------------------------------------------------------------------
-- Called by the C worker (esp_claw_init.c) for every incoming IPC message.
-- rule.trigger  : string — the IPC command name
-- rule.actions  : array  — flat strings (from claw_push_rule_to_lua)
-- rule.call_id  : string — OpenAI function call ID for response routing
-- rule.conditions: array — sensor condition tables (for automation rules)
function register_rule(rule)
    if type(rule) ~= "table" then return end
    local trigger = rule.trigger or ""

    -- -------------------------------------------------------------------------
    -- IR Hardware Callback: code successfully captured by RMT receiver
    -- -------------------------------------------------------------------------
    if trigger == "LUA_CMD_IR_LEARNED" then
        if rule.actions and rule.actions[1] then
            -- The C layer sends the code as a hex string: "0x00FF00FF"
            local hex_code = tonumber(rule.actions[1])  -- number for arithmetic

            if ir_learning_target_device and ir_learning_target_button then
                -- Ensure the device table exists in RAM
                ir_db[ir_learning_target_device] = ir_db[ir_learning_target_device] or {}

                -- Duplicate Detection
                -- ir_db stores strings; hex_code is a number.
                -- Re-parse each stored string to number before comparing.
                local is_duplicate = false
                local duplicate_button = nil
                for btn, stored_str in pairs(ir_db[ir_learning_target_device]) do
                    local stored_num = tonumber(stored_str)
                    if stored_num == hex_code and btn ~= ir_learning_target_button then
                        is_duplicate = true
                        duplicate_button = btn
                        break
                    end
                end

                if is_duplicate then
                    print("WARNING: Captured code matches existing button: " .. duplicate_button)
                    if c_inject_webrtc_message then
                        c_inject_webrtc_message(
                            "System Warning: Captured IR code is identical to the existing button '" ..
                            duplicate_button .. "'. Ask the user if they made a mistake, " ..
                            "want to overwrite it, or want to retry.")
                    end
                else
                    -- Store as canonical hex string — eliminates tostring() scientific notation.
                    local hex_str = string.format("0x%08X", hex_code)
                    ir_db[ir_learning_target_device][ir_learning_target_button] = hex_str
                    print("LEARNED: " .. ir_learning_target_device .. "." ..
                          ir_learning_target_button .. " -> " .. hex_str)

                    -- Auto-save: persist to LittleFS immediately on successful capture.
                    if ir.save_db then ir.save_db() end

                    if c_inject_webrtc_message then
                        c_inject_webrtc_message(
                            "System Context: IR code captured and saved successfully. " ..
                            "Ask the user if they want to test it by transmitting it back.")
                    end
                end

                -- Reset learning targets regardless of outcome
                ir_learning_target_device = nil
                ir_learning_target_button  = nil
            end
        end
        return

    -- -------------------------------------------------------------------------
    -- IR Hardware Callback: 15-second learning window expired
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_CMD_IR_TIMEOUT" then
        print("IR: Learning timed out.")
        ir_learning_target_device = nil
        ir_learning_target_button  = nil
        if c_inject_webrtc_message then
            c_inject_webrtc_message(
                "System Error: IR learning timed out. No code was received. " ..
                "Inform the user and ask if they want to retry.")
        end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: Arm IR learning mode for a specific device/button
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_IR_LEARN" then
        if rule.actions and rule.actions[1] and rule.actions[2] then
            ir_learning_target_device = rule.actions[1]
            ir_learning_target_button  = rule.actions[2]
            if ir.start_learning then
                ir.start_learning()
                if c_send_webrtc_response then
                    c_send_webrtc_response(rule.call_id,
                        "Armed. Ask the user to point the remote at the device and press the button now.")
                end
            else
                if c_send_webrtc_response then
                    c_send_webrtc_response(rule.call_id,
                        "Error: ir.start_learning C-binding not found.")
                end
            end
        else
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Error: ir_learn_button requires both device_name and button_name arguments.")
            end
        end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: Transmit a stored IR code
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_IR_TRANSMIT" then
        if rule.actions and rule.actions[1] and rule.actions[2] then
            local device = rule.actions[1]
            local button = rule.actions[2]
            local matched_dev, buttons = find_ir_device(device)
            if not buttons or not buttons[button] then
                if c_send_webrtc_response then
                    c_send_webrtc_response(rule.call_id,
                        "Error: Device '" .. tostring(device) .. "' or button '" .. tostring(button) ..
                        "' not found in the database.")
                end
            else
                if ir.send then
                    -- ir.send() C-binding calls strtoul(hex_str, NULL, 16).
                    -- Passing the "0x%08X" string directly is correct and safe.
                    ir.send(buttons[button])
                    if c_send_webrtc_response then
                        c_send_webrtc_response(rule.call_id, "Success: IR code transmitted.")
                    end
                else
                    if c_send_webrtc_response then
                        c_send_webrtc_response(rule.call_id,
                            "Error: ir.send C-binding not found.")
                    end
                end
            end
        else
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Error: ir_transmit_command requires both device_name and button_name arguments.")
            end
        end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: Explicit user-commanded save (optional; auto-save handles most cases)
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_IR_SAVE" then
        if ir.save_db then ir.save_db() end
        if c_send_webrtc_response then
            c_send_webrtc_response(rule.call_id, "Success: IR database saved to flash.")
        end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: Delete an IR device or a specific button (granular, nil-safe)
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_IR_DELETE" then
        -- rule.actions elements are flat strings (confirmed from claw_push_rule_to_lua).
        -- actions[1] = device_name (required)
        -- actions[2] = button_name (optional — if absent, deletes entire device)
        local device = rule.actions and rule.actions[1]
        local button = rule.actions and rule.actions[2]

        -- Guard: device argument is mandatory
        if not device or device == "" then
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Error: device_name argument is missing.")
            end
            return
        end

        -- Guard: device must exist in DB
        local matched_dev, buttons = find_ir_device(device)
        if not matched_dev then
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Error: Device '" .. tostring(device) .. "' not found in the database.")
            end
            return
        end

        if button and button ~= "" then
            -- Button-level deletion
            if not ir_db[matched_dev][button] then
                if c_send_webrtc_response then
                    c_send_webrtc_response(rule.call_id,
                        "Error: Button '" .. button .. "' not found on device '" .. matched_dev .. "'.")
                end
                return
            end

            ir_db[matched_dev][button] = nil

            -- Garbage-collect the device table if it is now empty
            if next(ir_db[matched_dev]) == nil then
                ir_db[matched_dev] = nil
                print("GC: removed empty device '" .. matched_dev .. "' from ir_db.")
            end

            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Button '" .. button .. "' deleted from device '" .. matched_dev .. "'.")
            end
        else
            -- Device-level deletion (all buttons removed)
            ir_db[matched_dev] = nil
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id,
                    "Device '" .. matched_dev .. "' and all its buttons have been deleted.")
            end
        end

        if ir.save_db then ir.save_db() end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: List all known IR devices and their buttons
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_IR_GET_DEVICES" then
        local response = ""
        local has_devices = false
        if ir_db then
            for device, buttons in pairs(ir_db) do
                has_devices = true
                local btn_list = ""
                for btn, _ in pairs(buttons) do
                    btn_list = btn_list == "" and btn or (btn_list .. ", " .. btn)
                end
                local entry = device .. " (buttons: " .. btn_list .. ")"
                response = response == "" and entry or (response .. "; " .. entry)
            end
        end
        if not has_devices then
            response = "No IR devices saved yet."
        else
            response = "Devices: " .. response .. "."
        end
        if c_send_webrtc_response then
            c_send_webrtc_response(rule.call_id, response)
        end
        return

    -- -------------------------------------------------------------------------
    -- System Command: List all automation rules
    -- -------------------------------------------------------------------------
    elseif trigger == "SYS_CMD:LIST" then
        local list_json = "["
        local first = true
        for k in pairs(rules_db) do
            if not first then list_json = list_json .. ", " end
            list_json = list_json .. '{"trigger": "' .. tostring(k) .. '"}'
            first = false
        end
        list_json = list_json .. "]"
        if c_send_webrtc_response then
            c_send_webrtc_response(rule.call_id, list_json)
        end
        return

    -- -------------------------------------------------------------------------
    -- System Command: Delete an automation rule
    -- -------------------------------------------------------------------------
    elseif trigger == "SYS_CMD:DELETE" then
        local target = rule.actions and rule.actions[1]
        if target and rules_db[target] then
            rules_db[target] = nil
            if c_save_rules then c_save_rules() end
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id, '{"status": "deleted"}')
            end
        else
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id, '{"error": "rule not found"}')
            end
        end
        return

    -- -------------------------------------------------------------------------
    -- LLM Tool: NetDiscovery Action Execution (Smart Macro & Dynamic Probe)
    -- -------------------------------------------------------------------------
    elseif trigger == "LUA_TOOL_NETDISCOVERY" then
        local action = (rule.actions and rule.actions[1]) or ""
        local target = (rule.actions and rule.actions[2]) or ""
        local room = (rule.actions and rule.actions[3]) or ""
        local entity_id = (rule.actions and rule.actions[4]) or ""
        local parameters_json = (rule.actions and rule.actions[5]) or "{}"

        print("LUA_TOOL_NETDISCOVERY: action='" .. action .. "' target='" .. target .. "' room='" .. room .. "' parameters_json='" .. parameters_json .. "'")

        if action == "" or target == "" then
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id, '{"error": "LUA_TOOL_NETDISCOVERY requires both action and target arguments."}')
            end
            return
        end

        if not c_send_intent then
            if c_send_webrtc_response then
                c_send_webrtc_response(rule.call_id, '{"error": "c_send_intent C-binding not found"}')
            end
            return
        end

        -- Unwrap double-nested WebRTC JSON parameters if present (e.g. {"name":"{...}","value":"{...}"})
        local function unwrap_parameters_json(json_str)
            if not json_str or json_str == "" or json_str == "{}" then return json_str end
            local inner_val = json_str:match('^%s*{%s*"name"%s*:%s*".-"%s*,%s*"value"%s*:%s*"(.-)"%s*}%s*$')
            if inner_val then
                return inner_val:gsub('\\"', '"'):gsub('\\\\', '\\')
            end
            return json_str
        end

        parameters_json = unwrap_parameters_json(parameters_json)

        -- Check if power_on_first is requested or if action is launch_app/open_app
        local power_requested = (string.find(parameters_json or "", '"power_on_first"%s*:%s*true') ~= nil) or
                                (action == "launch_app" or action == "open_app")

        -- 1. Resolve IP, service port, and manufacturer reachability trust status
        local ip, port, is_trusted = nil, 8080, false
        if c_get_entity_endpoint then
            ip, port, is_trusted = c_get_entity_endpoint(target)
        end
        if not port or type(port) ~= "number" or port <= 0 or port > 65535 then
            port = 8080
        end

        -- Helper: Bounded reachability probe loop with accurate elapsed time bookkeeping
        local function probe_reachability(target_ip, target_port, max_wait_ms, poll_interval_ms, probe_timeout_ms)
            if not target_ip or target_ip == "" or not target_port or target_port <= 0 then
                return false, 0
            end
            max_wait_ms = max_wait_ms or 5000
            poll_interval_ms = poll_interval_ms or 500
            probe_timeout_ms = probe_timeout_ms or 300

            local elapsed = 0
            while elapsed < max_wait_ms do
                if c_is_endpoint_ready and c_is_endpoint_ready(target_ip, target_port, probe_timeout_ms) then
                    return true, elapsed
                end
                c_sys_delay(poll_interval_ms)
                elapsed = elapsed + probe_timeout_ms + poll_interval_ms
            end
            return false, elapsed
        end

        -- ---------------------------------------------------------------------
        -- Special Handling: ATOMIC POWER_OFF
        -- ---------------------------------------------------------------------
        if action == "power_off" then
            print("[MACRO_PROBE] Processing power_off for target '" .. target .. "' (trust=" .. tostring(is_trusted) .. ")")

            local is_ready = false
            if ip and ip ~= "" and port and port > 0 then
                print("[MACRO_PROBE] Sondeando alcanzabilidad para power_off en " .. ip .. ":" .. tostring(port) .. " (Window: 5000ms)...")
                is_ready = probe_reachability(ip, port, 5000, 500, 300)
            end

            if is_trusted then
                -- CONFIRMED Manufacturer: Trust reachability state after full retry window
                if not is_ready then
                    print("[MACRO_PROBE] CONFIRMED: Device is consistently unreachable over network (5000ms window). Dispositivo ya está apagado.")
                    if c_send_webrtc_response then
                        c_send_webrtc_response(rule.call_id, '{"status": "info", "message": "El dispositivo ya se encuentra apagado."}')
                    end
                    return
                else
                    print("[MACRO_PROBE] CONFIRMED: Dispositivo alcanzable por red. Procediendo a apagar por IR.")
                end
            else
                -- UNCONFIRMED Manufacturer: Do NOT assert "ya está apagada" or skip IR based on reachability alone
                if not is_ready then
                    print("[MACRO_PROBE] UNCONFIRMED: Dispositivo inalcanzable por red. Estado no confirmado únicamente por red. Procediendo con señal IR.")
                else
                    print("[MACRO_PROBE] UNCONFIRMED: Dispositivo alcanzable por red. Procediendo con señal IR de apagado.")
                end
            end

            -- Helper to find IR power code via token/synonym matcher
            local power_code = nil
            local matched_dev, buttons = find_ir_device(target)
            if buttons and buttons["power"] then
                power_code = buttons["power"]
                print("[MACRO_PROBE] Código IR 'power' resuelto para '" .. target .. "' -> '" .. tostring(matched_dev) .. "' (" .. power_code .. ")")
            end

            if power_code and ir.send then
                ir.send(power_code)
                print("[MACRO_PROBE] Código IR enviado (" .. power_code .. ") a " .. target)
                if c_send_webrtc_response then
                    if not is_trusted then
                        c_send_webrtc_response(rule.call_id, '{"status": "hedged", "message": "No pude confirmar el estado exacto por red, pero envié la señal de apagado por infrarrojo."}')
                    else
                        c_send_webrtc_response(rule.call_id, '{"status": "success", "message": "Señal de apagado enviada correctamente."}')
                    end
                end
            else
                print("[MACRO_PROBE] AVISO: No se encontró código IR 'power' para " .. target .. " en ir_db.")
                if c_send_webrtc_response then
                    c_send_webrtc_response(rule.call_id, '{"status": "error", "message": "No se encontró un código IR registrado para apagar el dispositivo."}')
                end
            end
            return
        end

        -- Check if power_on_first is requested or if action is launch_app/open_app
        local power_requested = (string.find(parameters_json or "", '"power_on_first"%s*:%s*true') ~= nil) or
                                (action == "launch_app" or action == "open_app")

        if power_requested then
            -- 2. FAST INITIAL PROBE: Skip IR power ONLY if manufacturer reachability trust is CONFIRMED
            if is_trusted and ip and ip ~= "" and port and port > 0 then
                print("[MACRO_PROBE] Fast Probe previo (CONFIRMED) en " .. ip .. ":" .. tostring(port))
                if c_is_endpoint_ready and c_is_endpoint_ready(ip, port, 300) then
                    print("[MACRO_PROBE] HOT PATH (CONFIRMED): TV ya encendida y alcanzable en red. OMITIENDO SEÑAL IR POWER.")
                    c_send_intent(rule.call_id, action, target, room, entity_id, parameters_json)
                    return
                end
            elseif not is_trusted then
                print("[MACRO_PROBE] REACHABILITY UNCONFIRMED para '" .. target .. "'. Transmitiendo IR power por seguridad...")
            end

            -- 3. COLD PATH / UNCONFIRMED: Transmit IR power code.
            print("[MACRO_PROBE] Transmitiendo comando IR Power...")
            local power_code = nil
            local matched_dev, buttons = find_ir_device(target)
            if buttons and buttons["power"] then
                power_code = buttons["power"]
                print("[MACRO_PROBE] Código IR 'power' resuelto para '" .. target .. "' -> '" .. tostring(matched_dev) .. "' (" .. power_code .. ")")
            end

            if power_code and ir.send then
                ir.send(power_code)
                print("[MACRO_PROBE] Código IR enviado (" .. power_code .. ") a " .. target)
            else
                print("[MACRO_PROBE] AVISO: No se encontró código IR 'power' para " .. target .. " en ir_db.")
            end

            -- 4. Dynamic polling loop: Probe TCP port every 500ms until ready or 10s max timeout
            if ip and ip ~= "" and port and port > 0 then
                print("[MACRO_PROBE] Sondeando " .. ip .. ":" .. tostring(port) .. " (Max timeout: 10000ms)...")
                local is_ready, elapsed = probe_reachability(ip, port, 10000, 500, 300)
                if is_ready then
                    print("[MACRO_PROBE] Dispositivo listo en " .. elapsed .. "ms!")
                else
                    print("[MACRO_PROBE] Timeout de alcanzabilidad tras " .. elapsed .. "ms.")
                end
            else
                c_sys_delay(3000)
            end
        end

        print("[LUA_BRIDGE] Calling c_send_intent with parameters_json: " .. parameters_json)
        c_send_intent(rule.call_id, action, target, room, entity_id, parameters_json)
        return

    -- -------------------------------------------------------------------------
    -- System Commands: Execute / IR Direct (stub responses)
    -- -------------------------------------------------------------------------
    elseif trigger == "SYS_CMD:EXECUTE" or trigger == "SYS_CMD:IR_DIRECT" then
        if c_send_webrtc_response then
            c_send_webrtc_response(rule.call_id, '{"status": "executed"}')
        end
        return
    end

    -- -------------------------------------------------------------------------
    -- Standard Automation Rule Creation (fallback for unrecognized triggers)
    -- -------------------------------------------------------------------------
    rules_db = rules_db or {}
    rules_db[rule.trigger] = rule
    if c_save_rules then c_save_rules() end
    if c_send_webrtc_response and rule.call_id then
        c_send_webrtc_response(rule.call_id, "Automation rule created and saved.")
    end
end
