## Notes
- No clear reason to pick one board configuration vs another when creating the project. Still had to run `idf.py set-target ESP32-C6` afterwards to flash
- Had to add c_cpp_properties.json from the esp-zigbee-sdk .vscode examples for projects, otherwise intellisense was not working to find things like `esp_log.h`
- For good measure also copied the other files.