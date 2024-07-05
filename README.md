# Daz To Roblox Studio Developer Kit
A set of tools including a Daz Studio Plugin, Daz Scripts, Blender Scripts and asset files to aid in the conversion of Daz Studio Genesis characters to Roblox Avatars.

* Owner: [Daz 3D][OwnerURL] – [@Daz3d][TwitterURL]
* License: [Apache License, Version 2.0][LicenseURL] - see ``LICENSE`` and ``NOTICE`` for more information.
* Offical Release: [Daz to Roblox Studio][ProductURL]
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
The Daz To Roblox Studio Developer Kit can assist Roblox game developers to convert Genesis 9 characters into a format compatible with Roblox Studio for use as playable avatars.  This developer kit consists of a Daz Studio plugin, Daz Scripts, Blender Scripts and asset files which perform several operations including conversion to R15 rig, baking modesty coverings onto nude skin textures, baking separate images into a texture atlas, segmenting into separate mesh body parts and decimation of high resolution mesh data.  There is also support for generating S1 meshes compatible with Roblox Avatar Auto Setup.


## 2. Prerequisites
- A compatible version of the [Daz Studio][DazStudioURL] application
  - Minimum: 4.22
- A compatible version of the [Roblox Studio][CreateRobloxURL] application
- A compatible version of the [Blender][BlenderURL] application
  - Minimum: 3.6
- Operating System:
  - Windows 7 or newer
  - macOS 10.15 (Catalina) or newer


## 3. How to Download and Manually Install
1. If you have a previous version of this plugin installed, please run Daz Install Manager and uninstall then delete the previous package before continuing.
2. To uninstall a previous version, open the Daz Install Manager.  Then find the `DazToRobloxStudio Dev Kit` package in the `Installed` tab and click the corresponding `Uninstall` button.  You can delete the `DazToRobloxStudioDevKit.zip` by clicking the 3 dots and selecting `Delete the "DazToRobloxStudio Dev Kit" package`.
3. Download the appropriate `.package` file for your system (Win64 or Mac64) and the `.dsa` install script from the [Release Page][ReleasesURL].  Make sure that both files are in the same folder.
4. Double-click the `.dsa` install script file, named `DOUBLE_CLICK_ME_IM00098159-01_DazToRobloxStudioDevKit.dsa`.
5. Daz Studio launch and run the dsa install script.  The install script will automatically locate your Daz Install Manager archive folder and copy your package file into that folder.  If you are asked to overwrite an existing package file, click `Yes`.  Once the package file is copied to the correct folder, Daz Studio will automatically quit and Daz Install Manager will automatically start.
6. When Daz Install Manager starts, click the `Ready to Install` tab and look for `DazToRobloxStudio Dev Kit`.  If you have a previous version installed, `DazToRobloxStudio Dev Kit` may appear in the `Installed` tab.  Uninstall any previously installed `DazToRobloxStudio Dev Kit` package before continuing.
7. Select `DazToRobloxStudio Dev Kit` in the `Ready to Install` tab and click `Install`.
8. The product entry for `DazToRobloxStudio Dev Kit` should move from the `Ready to Install` tab to the `Installed` tab.
9. You may click the 3 dots next to `DazToRobloxStudio Dev Kit` to open additional menu options, then click `Show Installed Files` to find where your installed files are located.
10. To uninstall the `DazToRobloxStudio Dev Kit`, follow the instructions in Step 2.


## 4. How to Use
1. Start Daz Studio with an empty scene, then add the desired character to the scene.
2. Your character should be nude, without clothing or any accessories and in the default pose.
3. Make sure the top node of the character is selected in the Scene pane.
4. From the main menu, select File -> Send To -> Roblox Avatar Exporter.
4. A dialog will pop up: set the `Roblox Output Folder` line to wherever you want the final output files (FBX and Blender formats) to be saved.
5. Enable the Advanced Settings section if it is disabled, then make sure the `Blender Executable` line is set to the location of your blender executable. Example: `C:\Program Files\Blender Foundation\Blender 3.6\blender.exe`.
6. Set the desired asset name for the output.  This will be the filename of the final output files.
7. Click the Asset Type dropdown and select the desired setting: `Roblox R15 Avatar` will produce an FBX file that is ready for import using the existing Avatar Import method of Roblox Studio.  `Roblox S1 for Avatar Auto Setup` will produce an FBX file for use with the Roblox Avatar Auto Setup feature (currently available as a Beta Feature in Roblox Studio).  There are also tanktop variants of each setting, which will use a tanktop style modesty overlay instead of the default sportsbra modesty overlay.
8. If you are exporting a feminine Daz character, it is highly recommended to enable the `Breasts Gone` morph to reduce or remove the breast shape to help comply with Roblox Community Standards.  The `Breasts Gone` morph is part of the Genesis 9 Body Shapes add-on, which is a paid product available on the Daz Store.  If you do not have the Genesis 9 Body Shapes add-on, then you can try manually editing the mesh using Blender or Maya prior to importing into Roblox Studio.  Otherwise, it is recommended that you export only masculine Daz characters in order to comply with Roblox Community Standards.
9. Click the `Accept` button to proceed.  If you see an `Unable to Proceed` button, then may click it for detailed instructions on the missing steps needed to proceed.
10. After clicking the `Accept` button, you will be presented with the DazToRobloxStudio End User License Agreement (EULA).
11. After reading through the EULA, you may click `Accept EULA` to proceed with the conversion process.  Then wait for a dialog popup to notify you when the conversion is complete.
12. If the conversion completed successfully, click `OK` and a window to the Roblox Output Folder should automatically open.
13. If any errors occurred during conversion, you may be able to complete the conversion or find detailed error messages by running a script from the command-line.  Depending on your system setup, an explorer window or filesystem window should popup, showing the DazToRoblox Intermediate Folder.  Open a command-prompt or Terminal window and then run the `manual_blender_script.bat` on Windows or `sh manual_blender_script.sh` on Mac.  This script should start a command-line Blender process and give detailed feedback on any errors that are encountered.
14. If the conversion completed successfully, then an FBX and a Blender file should appear in the Roblox Output Folder.  The filenames should begin with the Asset Name from the Roblox Avatar Exporter window.  The FBX file is ready for import into Roblox Studio.  The Blender file can be opened in Blender to view and edit your character.
15. If you exported your character from Daz Studio using the `Roblox R15 Avatar` setting, then open Roblox Studio, click the Avatar tab, then click `Import 3D File`.  Select the final FBX output file to import, then click `Open`.  The filename should end with `_R15_ready_to_import.fbx`.
16. In the Import Preview window, click the `Rig General` section.  The Rig Type should be automatically set to `R15` and does not need to be changed.  Click the Rig Scale dropdown and select `Rthro Narrow`.  This will scale legacy Roblox accessories to be more compatible with realistic human body types like Daz characters.  You may try `Rthro` for masculine characters. NOTE: When uploading to Roblox with `Rthro Narrow` Rig Scale, there may be gaps between body parts. You must adjust your avatar's body build by going to Avatar -> Customize -> Head & Body -> Build and move the Body Type to the top left corner of the Body Type triangle.
17. If you exported your character from Daz Studio using the `Roblox S1 for Avatar Auto Setup` setting, then Open Roblox Studio.  If you have not enabled the Beta Feature for `Avatar Auto Setup` yet, then you can click File -> Beta Features, then scroll down to find `Avatar Auto-Setup Beta` and make sure it is enabled then click Save.  You may need to restart Roblox Studio.  Then click the Avatar tab and click `Import 3D File`.  Select the final FBX output file, then click `Open`.  The filename should end with `_S1_for_avatar_autosetup.fbx`.
18. In the Import Preview window, just click `Import`.  The character should appear in the scene and be automatically selected.  If the `Avatar Setup` pane is not open, click the `Avatar Setup` button in the Avatar tab.  In the `Avatar Setup` pane, you should see a `Set Up Avatar` button in the lower right corner.  Click the button and wait for Auto Setup to complete.
19.  When Auto Setup is complete, it should add a second copy of your character into the scene.  However, this new copy should be fully configured and functioning as a Roblox Avatar.
20. Alternatively, open the `.blend` file in the output folder to adjust cages and attachments and make desired modifications to the mesh or materials.  In addition to body segments, cages and attachments, this `.blend` file also contains a high resolution version of the character that can be used to bake normal maps and other operations.  This HD mesh will be set to invisible by default.
21. If using the `.blend` file from the final output folder, use the following settings when exporting to FBX:
- Path Mode: `Copy`, click icon to enable `Embed Textures`.
- Limit to: enable `Visible Objects`.
- Below the Object Types section, enable `Custom Properties`.
- In the Armature section, disable `Add Leaf Bones`.
- Enable Bake Animation, then open the section and disable `NLA Strips` and disable `All Actions`.
22. After exporting to an FBX, you can then import the FBX into Roblox Studio.


## 5. How to Build
Requirements:
- Daz Studio 4.5+ SDK, 
- Autodesk FBX SDK 2020.1 on Windows or FBX SDK 2015.1 on Mac, 
- Pixar OpenSubdiv 3.4.4 Library, 
- CMake, 
- C++ development environment

Download or clone the DazToRoblox github repository to your local machine. The Daz Bridge Library is linked as a git submodule to the DazBridge repository. Depending on your git client, you may have to use `git submodule init` and `git submodule update` to properly clone the Daz Bridge Library.  FBX SDK is available as an installer file downloadable from the Autodesk website.  Be sure to only use the correct version for your system mentioned in the Requirements above.  OpenSubdiv should be configured and built as a static CPU only library.  For convenience, pre-built libraries for OpenSubdiv 3.4.4 for both Win64 and Mac64 are available here: https://github.com/danielbui78/OpenSubdiv/releases/tag/v3.4.4

Use CMake to configure the project files. Daz Bridge Library will be automatically configured to static-link. If using the CMake gui, you will be prompted for folder paths to dependencies: Daz SDK (DAZ_SDK_DIR), FBX SDK (FBX_SDK_DIR) and OpenSubdiv (OPENSUBDIV_DIR) during the Configure process.  NOTE: Use only the version of Qt 4.8 included with the Daz SDK.  Any external Qt 4.8 installations will most likely be incompatible with Daz Studio development.
- For the DAZ_SDK_DIR, then default Windows path should be similar to `C:/Users/Public/Documents/My DAZ 3D Library/DAZStudio4.5+ SDK`.  On Mac, this should be similar to `/Users/Shared/My DAZ 3D Library/DAZStudio4.5+ SDK`.
- For the FBX_SDK_DIR, the default Windows path should be similar to `C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1`.  On Mac, this should be similar to `/Applications/Autodesk/FBX SDK/2015.1`.
- For OPENSUBDIV_DIR, if you are using the prebuilt libraries from the link above, you will only need to specify the root folder, example: `C:/Users/<username>/OpenSubdiv-3.4.4` or `/Users/<username>/OpenSubdiv-3.4.4`.  Otherwise, if you built from source, you will need to manually specify the OPENSUBDIV_INCLUDE folder path, ex: `C:/Users/<username>/OpenSubdiv-3.4.4`, and the OPENSUBDIV_LIB file path, ex: `C:/Users/<username>/OpenSubdiv-3.4.4/build/lib/Release/osdCPU.lib`.


## 6. How to QA Test
To Do:
1. Write `QA Manual Test Cases.md` for DazToRoblox using [Example QA Manual Test Cases.md](https://github.com/daz3d/DazToC4D/blob/master/Test/Example%20QA%20Manual%20Test%20Cases.md).
2. Implement the manual tests cases as automated test scripts in `Test/TestCases`.
3. Update `Test/UnitTests` with latest changes to DazToRoblox class methods.

The `QA Manual Test Cases.md` document should contain instructions for performing manual tests.  The Test folder also contains subfolders for UnitTests, TestCases and Results. To run automated Test Cases, run Daz Studio and load the `Test/testcases/test_runner.dsa` script, configure the sIncludePath on line 4, then execute the script. Results will be written to report files stored in the `Test/Reports` subfolder.

To run UnitTests, you must first build special Debug versions of the DazToRoblox and DzBridge Static sub-projects with Visual Studio configured for C++ Code Generation: Enable C++ Exceptions: Yes with SEH Exceptions (/EHa). This enables the memory exception handling features which are used during null pointer argument tests of the UnitTests. Once the special Debug version of DazToRoblox dll is built and installed, run Daz Studio and load the `Test/UnitTests/RunUnitTests.dsa` script. Configure the sIncludePath and sOutputPath on lines 4 and 5, then execute the script. Several UI dialog prompts will appear on screen as part of the UnitTests of their related functions. Just click OK or Cancel to advance through them. Results will be written to report files stored in the `Test/Reports` subfolder.

For more information on running QA test scripts and writing your own test scripts, please refer to `How To Use QA Test Scripts.md` and `QA Script Documentation and Examples.dsa` which are located in the Daz Bridge Library repository: https://github.com/daz3d/DazBridgeUtils.

Special Note: The QA Report Files generated by the UnitTest and TestCase scripts have been designed and formatted so that the QA Reports will only change when there is a change in a test result.  This allows Github to conveniently track the history of test results with source-code changes and allows developers and QA testers to notified by Github or their git client when there are any changes and the exact test that changed its result.


## 7. How to Modify and Develop
The Daz Studio Plugin source code is contained in the `DazStudioPlugin` folder. The Daz Script, Blender python source code, overlay images and other data files used by the Daz Studio Plugin are in the `PluginData` folder.  Currently, the files need to be zip compressed into a file named `plugindata.zip` and placed in the `DazStudioPlugin/Resources` folder prior to building the DLL.  The `plugindata.zip` will be embedded into the `dzroblox.dll` plugin file.

The DazToRoblox Daz Studio plugin uses a branch of the Daz Bridge Library which is modified to use the `DzRobloxNS` namespace. This ensures that there are no C++ Namespace collisions when other plugins based on the Daz Bridge Library are also loaded in Daz Studio. In order to link and share C++ classes between this plugin and the Daz Bridge Library, a custom `CPP_PLUGIN_DEFINITION()` macro is used instead of the standard DZ_PLUGIN_DEFINITION macro and usual .DEF file. NOTE: Use of the DZ_PLUGIN_DEFINITION macro and DEF file use will disable C++ class export in the Visual Studio compiler.


## 8. Directory Structure
The directory structure is as follows:

- `DazLibraryFiles`:          Asset files to be installed into the user's Daz Studio Content Library.
- `DazStudioPlugin`:          Files that pertain to the Daz Studio plugin.
- `DevKit`:                   Supplemental asset files to assist with creating a Roblox avatar.
- `dzbridge-common`:          Files from the Daz Bridge Library used by Daz Studio plugin.
- `InternalAssets`:           Internal development and production files.
- `PluginData`:               Scripts, assets and other data files which are used by the Daz Studio plugin.
- `Test`:                     Scripts and generated output (reports) used for Quality Assurance Testing.

[OwnerURL]: https://www.daz3d.com
[TwitterURL]: https://twitter.com/Daz3d
[LicenseURL]: http://www.apache.org/licenses/LICENSE-2.0
[ProductURL]: https://www.daz3d.com/daz-to-roblox
[RepositoryURL]: https://github.com/daz3d/DazToRoblox/
[DazStudioURL]: https://www.daz3d.com/get_studio
[ReleasesURL]: https://github.com/daz3d/DazToRoblox/releases
[CreateRobloxURL]: https://create.roblox.com/
[BlenderURL]: https://www.blender.org/download

