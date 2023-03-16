module("luci.controller.log_daemon_controller", package.seeall)

function index()
    entry({"admin", "services", "log_daemon"}, cbi("log_daemon_model"), "Tuya cloud connection", 10)
end