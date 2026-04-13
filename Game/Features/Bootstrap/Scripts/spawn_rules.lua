-- Bootstrap spawn rules module.
-- The current scripting host validates this module contract and routes the
-- declared entry points through gameplay commands. A future Lua backend will
-- execute these functions behind the same IScriptingService boundary.

local M = {}

function M.request_spawn(payload)
    return payload
end

return M
