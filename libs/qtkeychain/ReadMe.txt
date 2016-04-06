QtKeychain
==========

QtKeychain is a Qt API to store passwords and other secret data securely. How the data is stored depends on the platform:

 * **Mac OS X:** Passwords are stored in the OS X Keychain.

 * **Linux/Unix:** If running, GNOME Keyring is used, otherwise qtkeychain tries to use KWallet (via D-Bus), if available.

 * **Windows:** By default, the Windows Credential Store is used (requires Windows 7 or newer).
Pass -DUSE_CREDENTIAL_STORE=OFF to cmake use disable it. If disabled, QtKeychain uses the Windows API function
[CryptProtectData](http://msdn.microsoft.com/en-us/library/windows/desktop/aa380261%28v=vs.85%29.aspx "CryptProtectData function")
to encrypt the password with the user's logon credentials. The encrypted data is then persisted via QSettings.

In unsupported environments QtKeychain will report an error. It will not store any data unencrypted unless explicitly requested (setInsecureFallback( true )).

**License:** QtKeychain is available under the [Modified BSD License](http://www.gnu.org/licenses/license-list.html#ModifiedBSD). See the file COPYING for details.
