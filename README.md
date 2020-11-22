# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore
## BetterItemReloading
- Latest build status with azerothcore: [![Build Status](https://github.com/azerothcore/mod-better-item-reloading/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-better-item-reloading)

BetterItemReloading is a C++ Azerothcore module which allows to reload items on the server side and as much as possible on the client side of WoW 3.3.5.<br>
<br>
Sadly some things are cached on the client which can't be properly invalidated and need DBC file changes.<br>
The following things must be changed in DBC files: <br>
* ItemClass
* ItemSubClass
* sound_override_subclassid
* MaterialID
* ItemDisplayInfo
* InventorySlotID
* SheathID
<br>
Multiple items can be reloaded by splitting each entry id with a space like: .breload item 12345 23456 34567
<br>
<br>
Here's an example how i change an item on the fly in the database and reload it without restarting anything:<br>
<p>
    <img src="Example.gif" height="747" width="595" />
</p>
