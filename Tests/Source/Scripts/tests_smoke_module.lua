-- Smoke-test script module used to validate the stable scripting host
-- contract. The current host does not execute Lua yet; it validates this path
-- and exposes the declared entry points for load/invoke testing.

local M = {}

function M.emit_smoke_command(payload)
    return payload
end

return M
