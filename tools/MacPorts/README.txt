* Download Macports [dports-dev](http://downloads.natron.fr/Third_Party_Sources/dports-dev.zip)

* Give read/execute permissions to the local repository (replace `USER_NAME` with your user name):
```
chmod 755 /Users/USER_NAME/Development/Naton/tools/MacOSX/ports
```

* Check that the user "nobody" can read this directory by typing the following command in a Terminal: `sudo -u nobody ls /Users/USER_NAME/Development/Naton/tools/MacOSX/ports`. If it fails, then try again after having given execution permissions on your home directory using the following command: `chmod o+x /Users/USER_NAME`. If this still fails, then something is really wrong.

* Edit the sources.conf file for MacPorts, for example using the nano editor: `sudo nano /opt/local/etc/macports/sources.conf`, insert at the beginning of the file the configuration for a local repository (read the comments in the file), by inserting the line `file:///Users/USER_NAME/Development/Naton/tools/MacOSX/ports` (without quotes, and yes there are *three* - 3 - slashes). Save and exit (if you're using nano, this means typing ctrl-X, Y and return).

* Update MacPorts:
```
sudo port selfupdate
```

* Recreate the index in the local repository: (no need to be root for this)
```
cd /Users/USER_NAME/Development/Naton/tools/MacOSX/ports; portindex"
```
