# Daz To Roblox UGC Developer Kit
A set of tools including a Daz Studio Plugin, Daz Scripts, Blender Scripts and asset files to aid in the conversion of Daz Studio Genesis characters to Roblox Avatars.

* Owner: [Daz 3D][OwnerURL] – [@Daz3d][TwitterURL]
* License: [Apache License, Version 2.0][LicenseURL] - see ``LICENSE`` and ``NOTICE`` for more information.
* Offical Release: [Daz to Roblox][ProductURL]
* Official Project: [github.com/daz3d/DazToRoblox][RepositoryURL]


## Table of Contents
1. About the Developer Kit
2. Prerequisites
3. How to Install
4. How to Use
5. How to Build
6. How to QA Test
7. How to Develop
8. Directory Structure


## 1. About the Developer Kit
The Daz To Roblox UGC Developer Kit can assist Roblox UGC developers to convert Genesis 9 characters into a format compatible with Roblox Studio for use as playable avatars.  This developer kit consists of a Daz Studio plugin, Daz Scripts, Blender Scripts and asset files which perform several operations including conversion to R15 rig, baking modesty coverings onto nude skin textures, baking separate images into a texture atlas, segmenting into separate mesh body parts and decimation of high resolution mesh data.


## 2. Prerequisites
- A compatible version of the [Daz Studio][DazStudioURL] application
  - Minimum: 4.20
- A compatible version of the [Roblox Studio][CreateRobloxURL] application
- A compatible version of the [Blender][BlenderURL] application
  - Minimum: 3.6
- Operating System:
  - Windows 7 or newer
  - macOS 10.15 (Catalina) or newer


## 3. How to Download and Manually Install
1. Download the developer kit zip file from the [Release Page][ReleasesURL].
2. Move the `DazToRobloxUGCDevKitWin64.zip` file to your `DAZ 3D\InstallManager\Downloads\` folder.  This is commonly located in `C:\Users\Public\Documents\DAZ 3D\InstallManager\Downloads`.  Mac users should use `DazToRobloxUGCDevKitMac64.zip`.
3. Run the Daz Install Manager, go to the Ready to Install tab.
4. Find `DazToRoblox UGC Dev Kit` and click `Install`.
5. The product entry for `DazToRoblox UGC Dev Kit` should move from the Ready to Intall tab to the Installed tab.
6. You may click the 3 dots next to `DazToRoblox UGC Dev Kit` to open additional menu options, then click `Show Installed Files` to find where your installed files are located.
7. To uninstall, click the `Uninstall` button from the Daz Install Manager.  You can delete the `DazToRobloxUGCDevKit.zip` by clicking the 3 dots and selecting `Delete the "DazToRoblox UGC Dev Kit" package`.


## 4. How to Use
1. Open your character in Daz Studio.
2. Make sure top node of the character is selected in the Scene pane.
3. From the main menu, select File -> Send To -> Daz To Roblox.
4. A dialog will pop up: set the `Roblox Output Folder` line to wherever you want the final output file to be saved.
5. Enable the Advanced Settings section if it is disabled, then make sure the `Blender Executable` line is set to the location of your blender executable. Example: `C:\Program Files\Blender Foundation\Blender 3.6\blender.exe`.
6. Set the desired asset name for the output.
7. Click the Asset Type dropdown and select the desired modesty coverings for your avatar.  WARNING: Please make sure you adhere to Roblox Community Standards, especially prohibited Sexual Content.
8. Click Accept, then wait for a dialog popup to notify the conversion is complete.
9. A window to the Roblox Output Folder should automatically open.
10. You may now open the final FBX output file in your preferred editor, such as Blender or Maya.  Adjust cages and attachments and make any desired modifications.  This FBX file is in the correct scale and orientation for import into Roblox Studio.
11. Alternatively, open the .blend file in the output folder to adjust cages and attachments and make desired modifications.
12. If using the .blend file from the final output folder, use the following settings when exporting to FBX:
- Path Mode: `Copy`, click icon to enable `Embed Textures`.
- In the Armature section, disable `Add Leaf Bones`.
13. If you want to work with intermediate files, click the `Open Intermediate Folder` in the Advanced Settings of the Send To -> Daz To Roblox dialog popup.


## 5. How to Build
Requirements: Daz Studio 4.5+ SDK, Autodesk Fbx SDK 2020.1 on Windows or 2015 on Mac, Pixar OpenSubdiv Library, CMake, C++ development environment

Download or clone the DazToRoblox github repository to your local machine. The Daz Bridge Library is linked as a git submodule to the DazBridge repository. Depending on your git client, you may have to use `git submodule init` and `git submodule update` to properly clone the Daz Bridge Library.

Use CMake to configure the project files. Daz Bridge Library will be automatically configured to static-link with DazToBlender. If using the CMake gui, you will be prompted for folder paths to dependencies: Daz SDK, Fbx SDK and OpenSubdiv during the Configure process.  NOTE: Use only the version of Qt 4.8 included with the Daz SDK.  Any external Qt 4.8 installations will most likely be incompatible with Daz Studio development.


## 6. How to QA Test
To Do:
1. Write `QA Manaul Test Cases.md` for DazToRoblox using [Example QA Manual Test Cases.md](https://github.com/daz3d/DazToC4D/blob/master/Test/Example%20QA%20Manual%20Test%20Cases.md).
2. Implement the manual tests cases as automated test scripts in `Test/TestCases`.
3. Update `Test/UnitTests` with latest changes to DazToBlender class methods.

The `QA Manaul Test Cases.md` document should contain instructions for performing manual tests.  The Test folder also contains subfolders for UnitTests, TestCases and Results. To run automated Test Cases, run Daz Studio and load the `Test/testcases/test_runner.dsa` script, configure the sIncludePath on line 4, then execute the script. Results will be written to report files stored in the `Test/Reports` subfolder.

To run UnitTests, you must first build special Debug versions of the DazToRoblox and DzBridge Static sub-projects with Visual Studio configured for C++ Code Generation: Enable C++ Exceptions: Yes with SEH Exceptions (/EHa). This enables the memory exception handling features which are used during null pointer argument tests of the UnitTests. Once the special Debug version of DazToBlender dll is built and installed, run Daz Studio and load the `Test/UnitTests/RunUnitTests.dsa` script. Configure the sIncludePath and sOutputPath on lines 4 and 5, then execute the script. Several UI dialog prompts will appear on screen as part of the UnitTests of their related functions. Just click OK or Cancel to advance through them. Results will be written to report files stored in the `Test/Reports` subfolder.

For more information on running QA test scripts and writing your own test scripts, please refer to `How To Use QA Test Scripts.md` and `QA Script Documentation and Examples.dsa` which are located in the Daz Bridge Library repository: https://github.com/daz3d/DazBridgeUtils.

Special Note: The QA Report Files generated by the UnitTest and TestCase scripts have been designed and formatted so that the QA Reports will only change when there is a change in a test result.  This allows Github to conveniently track the history of test results with source-code changes, and allows developers and QA testers to notified by Github or their git client when there are any changes and the exact test that changed its result.


## 7. How to Modify and Develop
The Daz Studio Plugin source code is contained in the `DazStudioPlugin` folder. The Daz Script, Blender python source code, overlay images and other data files used by the Daz Studio Plugin are in the `PluginData` folder.  Currently, the files need to be zip compressed into a file named `plugindata.zip` and placed in the `DazStudioPlugin/Resources` folder prior to building the DLL.  The `plugindata.zip` will be embedded into the `dzroblox.dll` plugin file.

The DazToRoblox Daz Studio plugin uses a branch of the Daz Bridge Library which is modified to use the `DzRobloxNS` namespace. This ensures that there are no C++ Namespace collisions when other plugins based on the Daz Bridge Library are also loaded in Daz Studio. In order to link and share C++ classes between this plugin and the Daz Bridge Library, a custom `CPP_PLUGIN_DEFINITION()` macro is used instead of the standard DZ_PLUGIN_DEFINITION macro and usual .DEF file. NOTE: Use of the DZ_PLUGIN_DEFINITION macro and DEF file use will disable C++ class export in the Visual Studio compiler.


## 8. Directory Structure
The directory structure is as follows:

- `DazLibraryFiles`:          Asset files to be installed into the user's Daz Studio Content Library.
- `DazStudioPlugin`:          Files that pertain to the Daz Studio plugin.
- `dzbridge-common`:          Files from the Daz Bridge Library used by Daz Studio plugin.
- `InternalAssets`:           Internal development and production files.
- `PluginData`:               Scripts, assets and other data files which are used by the Daz Studio plugin.
- `Test`:                     Scripts and generated output (reports) used for Quality Assurance Testing.
- `UgcDevKit`:                Supplemental asset files to assist with creating a Roblox avatar.

[OwnerURL]: https://www.daz3d.com
[TwitterURL]: https://twitter.com/Daz3d
[LicenseURL]: http://www.apache.org/licenses/LICENSE-2.0
[ProductURL]: https://www.daz3d.com/daz-to-roblox
[RepositoryURL]: https://github.com/daz3d/DazToRoblox/
[DazStudioURL]: https://www.daz3d.com/get_studio
[ReleasesURL]: https://github.com/daz3d/DazToRoblox/releases
[CreateRobloxURL]: https://create.roblox.com/
[BlenderURL]: https://www.blender.org/download

