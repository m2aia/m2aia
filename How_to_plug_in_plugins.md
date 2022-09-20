 1. create the plugin using MITK's PluginGenerator
 2. copy the created plugin from /tmp/<plugin-symbolic-name> to M2aia/Plugins
 3. edit the following files to contain the plugins symbolic name:
  - `M2aia/Plugins/PluginList.cmake`
    - the ":ON"-switch tells cmake-gui whether the plugin should be built by default. Choose ":Off" to disable the checkbox in cmake-gui
  - `M2aia/Applications/M2aiaWorkbench/target_libraries.cmake`
    - the `.` in the plugin symbolic name have to be replaced with `_`. 
  - `M2aia/CMake/BuildConfigurations/M2aiaRelease.cmake`
 4. reconfigure using first cmake-gui and recompile using make
 5. start M2aiaWorkbench
