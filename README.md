# dx_subsystem_template
dx_subsystem_template

## Changes to Make ##
- [ ] Rename the `dx_subsystem*` folders to the same name as this subsystem with the same prefixes as in the template
- [ ] If there are no messages and no commander for the subsystem, att `CATKIN_IGNORE` in the `_msgs` and `_commander` folders
- [ ] Search and replaces all instances of `dx_subsystem` with the name of your subsystem
- [ ] Fill out `README.md` in the (renamed) `dx_subsystem` folder
- [ ] Update all package.xmls with maintainers, descriptions, and dependencies
- [ ] Update all `CMakeLists.txt` - remove python parts or C++ parts if not needed, add dependencies
- [ ] Remove this list