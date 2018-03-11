# prepatch

## What is this?
  Prepatch is an alternative to [patchmanager](http://talk.maemo.org/showthread.php?t=92935).  
  However unlike patchmanager prepatch doesn't modify any files on disk but instead replaces the "open"/"open64" function in order to make applications **think** that those files were modified.  
  
## Why should I use/not use prepatch instead of patchmanger?
### Advantages

* Undoing patches is never a problem even if multiple patches were applied to a single file.  
* Patches don't have to be reapplied if the patched file has been updated.
* System-Updates no longer break stuff if patches are applied and all patches which still work after the update will instantly be working again (no reenabling patches).
* An "apply-order" can be specified if patches which patch the same file need it.
* prepatch can replace/add files if you're confident that the patched file should stay the same even if jolla updates it.
* It's generally "cleaner" to leave files of other applications alone.

### Disadvantages

* Because of the way it works prepatch will slow down the opening of all files a (small) bit since it has to check if patches have to be applied and, if necessary, apply them. Patchmanager doesn't suffer from this, since it applies the patch to the real files.
* No gui (at the moment) to install/toggle patches. However this will likely change in the future
* Patchmanager has a lot more patches availible. However porting them to prepatch should be trivial in the vast majority of cases.

## How does it work?

Prepatch works by preloading a library into every binary (don't worry, the overheat is much smaller then it sounds) and replaces the open+open64 function in there.

If an application tries to open a file which doesn't have a patch (or tries to open the file for writing/as any user other then nemo) the replaced open function will just behave like the normal open function.

However if an application tries to open a file for which a patch has been installed (in most cases an qml-file) prepatch will apply the patch(es) to a temporary copy of the file and then open that one instead of the one the application requested.

That way the application will **think** that the file has been patched and behave that way while in reality the real file stays unmodified.

## How do I install it?

Please note that this hasn't been packaged yet so if you want to use it in it's current state you'll have to build and setup it yourself in yourself:
```
devel-su pkcon refresh
devel-su pkcon install git make gcc patch
cd #These instructions assume that you're cloning the repo to your home-folder
git clone https://github.com/jakibaki/prepatch
cd prepatch
make
devel-su
echo "/home/nemo/prepatch/libprepatch.so" >> /etc/ld.so.preload
exit
```

## How do I uninstall it?

To uninstall prepatch and all patches that were installed using it simply run

```
devel-su
rm /etc/ld.so.preload 
rm -r /usr/share/prepatch
```

and you're done.

## How do I install patches?

As this hasn't been packaged yet you'll have to install patches manually right now.  
In the future it should be possible to simply install a package using storeman/pkcon.

As an example this is the process of installing my keyboard-swipe-patch (after following the installation-instructions above):

```
cd
git clone https://github.com/jakibaki/sailfishos-prepatch-keyboard-swipe
cd sailfishos-prepatch-keyboard-swipe
devel-su cp -r 050-prepatch-keyboard-swipe /usr/share/prepatch/
killall maliit-server # This restarts the keyboard
```

For uninstalling the patch simply remove `/usr/share/prepatch/050-prepatch-keyboard-swipe` and run `killall maliit-sever` to restart the keyboard.

## How do I develop patches?

Developing patches for prepatch is easy :) 

I'll explain it with my keyboard-swipe-patch as the example. Replace the names/path with yours as needed.

First we'll create the patch-folder (which will be placed into /usr/share/prepatch after we're done creating the patch).

```
mkdir 050-prepatch-keyboard-swipe
```

The `050` is there to make it easy to specify the order in which the patches shall be loaded.

If for example an other patch modifies the same file as yours but only works if your patch is applied before the other one you simply choose an lower number and your patch will always be applied first.

Don't cd into it yet.

First create two copies of the file which you want to modify.  
One will stay the same and the other one will be modified.

```
cp /usr/share/maliit/plugins/com/jolla/KeyboardBase.qml KeyboardBase-Original.qml
cp /usr/share/maliit/plugins/com/jolla/KeyboardBase.qml KeyboardBase-Modified.qml
```

Please note that, if there's already a prepatch patch applied on this file you'll get the modified version when trying to copy it (as prepatch is loaded into `cp`).  
If that's the case temporarily disable the other patch until you're done with this step.

Now edit the KeyboardBase-Modified.qml file according to your needs.

After that create the directory in which your patch will be located in.

In my case I have to run

```
mkdir -p 050-prepatch-keyboard-swipe/usr/share/maliit/plugins/com/jolla/
```

Now run `diff -u old new > 050-yourpatch/path/to/orig.qml.patch`.

Im my case:

```
diff -u KeyboardBase-Original.qml KeyboardBase-Modified.qml > 050-prepatch-keyboard-swipe/usr/share/maliit/plugins/com/jolla/KeyboardBase.qml.patch
```

Repeat this process with all the files you want to patch (you can place multiple patches in the same 050-prepatch-folder).

If you want to add/completely replace a file instead of patching it you can simply place it inside the folder without the `.patch` extension. Please note that new files will not appear in directory-listings so `ls /path/to/fakefile` will not show the file but running `cat /path/to/fakefile/file` will print out its contents.

Now simply copy the `050-prepatch*` folder to /usr/share/prepatch and your patch will be applied the next time the modified file is loaded!

## Is it possible to run this alongside with patchmanager?

Yes it is. However all patches that are applied using prepatch will be "applied" *after* the patchmanager-patches.

## What bugs/issues are currently known?

* For some reason after starting/restarting lipstick it keeps loading for about 20 seconds.  
After that everything goes back to normal and performance doesn't seem to be affected in other apps so I suspect that this is caused by a bug in prepatch.

## I want to help! How can I help?

* Prepatch hasn't been packaged into an rpm yet so any help with that and/or creating a template for patch-packages would be greatly appreciated!
* There's no graphical interface for toggeling patches yet.

## What patches are availible?

* [keyboard-swipe-cursor](https://github.com/jakibaki/sailfishos-prepatch-keyboard-swipe) by me (jakibaki).  
  It's a simple patch which allows you to move the cursor on the keyboard by swiping it to the left/right.
  
If you want your patch listed here please open an issue or write to me on Telegram (@jakibaki).
