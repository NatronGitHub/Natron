

Merge: never consider RGB as being transparent by default - this is OK for unpremultiplied compositing (After Effects) but is invalid in a premultiplied compositor such as Natron or Nuke. Users still have the option to ignore the alpha channel. (new from v2.4)