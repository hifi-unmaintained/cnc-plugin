cnc-plugin - stuffin' C&C in your browser
=========================================
cnc-plugin is a browser plugin for all NPAPI supporting browsers to encapsulate a cnc-ddraw supported game in any web page.

Installing
----------
To successfully register the plugin, following is required:

  1. Copy npcncplugin.dll as <whatever>.dll you want to <wherever> you want
  2. Add a configuration file named "<whatever>.ini" to <wherever> you placed the dll containing information where to download updates from
  3. The plugin will create a directory based on the "game" <param /> in <wherever> to install the app data

The plugin should now be ready to install/update the application.

Updating works by checking version.txt from the server and comparing the first line's integer (which is the version number) and then downloading all files in the set that do not have the same SHA1 sum as the current file on disk if it exists. If any of the files end in .gz they are extracted after downloading. A special case would be an actual gzip file but you'll need to compensate that by doing double compression, file.gz.gz, sorry.

After the application is at the current version, the executable defined in cncplugin.ini is launched.

If version.txt and the actual files are not in sync cnc-plugin will refuse to launch the application because of a checksum error. Only way to get around this is to restart the plugin (by refreshing the page) and re-downloading the failed files. cnc-plugin ensures this by removing local version.txt when a new one is successfully downloaded and refuses to start the application without one.

cnc-plugin itself will NEVER be updated automatically this way. Preferred way to update cnc-plugin is to wrap up installing it in a browser *extension* which can be kept up-to-date using the vendors own channel.

Example npcncplugin.ini
---------------------
[cnc95]
application=Command & Conquer
executable=C&C95.exe
url=http://example.com/plugin-data/cnc95/release.txt

[ra95]
application=Command & Conquer: Red Alert
executable=ra95.exe
url=http://example.com/plugin-data/ra95/release.txt

Web server file structure
-------------------------
path/to/data/
    release.txt
        -- snip --
        <version>   (converted with atoi() for comparison, be careful)
        [hash1] [file1]
        [hash2] [file2]
        ...
        -- snap --
    app.exe.gz
    data.mix.gz
    lib.dll.gz
    uncomp.txt.gz
