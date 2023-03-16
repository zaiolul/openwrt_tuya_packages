map = Map("log_daemon", "Tuya cloud connection")

enableSection = map:section(NamedSection, "log_daemon_sct", "log_daemon", "Enable program")
flag = enableSection:option(Flag, "enable", "Enable", "Enable program")
dataSection = map:section(NamedSection, "log_daemon_sct", " log_daemon", "Device info")
product = dataSection:option(Value, "product_id", "Product ID")

device = dataSection:option(Value, "device_id", "Device ID")
secret = dataSection:option(Value, "secret", "Device secret")
product.size = 30
product.maxlength = 30
device.size = 30
device.maxlength = 30
secret.size = 30
secret.maxlength = 30

return map