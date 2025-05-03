## Notes
- No clear reason to pick one board configuration vs another when creating the project. Still had to run `idf.py set-target esp32c6` afterwards to flash
- Had to add c_cpp_properties.json from the esp-zigbee-sdk .vscode examples for projects, otherwise intellisense was not working to find things like `esp_log.h` This can be done with Ctrl+Shift+P and then searching for "ESP-IDF add vscode configuration folder" folder
- The thing would not build until I enabled zigbee in sdkconfig, this isn't enabled in this extension version by default when you generate the project
- Zigbee configuratiuon needs to include router device in sdkconfig if you are using a router device
- partitions.csv is required for this version of the SDK (5.3.2)