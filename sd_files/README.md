###### You can keep this file on your sd card for later reference or delete it

### Notes
Every folder to be registered requries a manifest.lua file example contents :

```
return {
    type = "app",               # "app" or "folder"
    name = "App 1",
    icon = "icon_code",         # see full list below
    entry = "main.lua",         # name of the main .lua file of the app, remove for folders
    priority = 50               # default is 100 wifi app is 10 and config 20
}
```

### Example sd card structure

(see in the singularity folder above)

```
/singularity/
├── apps/
│   ├── app1/
│   │   ├── manifest.lua
│   │   └── main.lua
│   ├── app2/
│   │   ├── manifest.lua        # type = "app"
│   │   └── main.lua
│   └── group1/
│       ├── manifest.lua        # type = "folder"
│       └── app3/
│           ├── manifest.lua    
│           └── main.lua
└── system/
    ├── config.lua              # created on first config edit
    └── wifi_networks.lua       # created on first network entry
```

### Icons

more will be added as updates come

| Name | Description |
|---|---|
| `icon_globe` | Globe / network |
| `icon_cog` | Settings / gear |
| `icon_msdicon` | microSD |
| `icon_iricon` | Infrared |
| `icon_command` | Terminal / command |
| `icon_code` | Code / brackets |
| `icon_file` | Generic file (default fallback) |
| `icon_bluetooth` | Bluetooth |
| `icon_tool` | Tools / utility |
