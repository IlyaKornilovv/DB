# DB Workbench GUI

Windows/WPF client for the C++ DB server. The GUI talks to the server directly over the same TCP protocol as the native client, so no `db_native.dll` is required.

## Run like in the demo video

1. Build and start the server from WSL/Linux project root:

```bash
./scripts/build_full.sh
./build/customdb_server --host 0.0.0.0 --port 5432
```

2. Open Windows PowerShell in this folder and run:

```powershell
dotnet run
```

3. In the window use `localhost` and port `5432`, click **Connect**, then **Execute**.


Troubleshooting: this version starts QueryWorkbench from Program.cs and does not rely on StartupUri.


Troubleshooting: this build initializes SQL examples after the WPF visual tree is loaded, so startup no longer fails on SelectionChanged.
