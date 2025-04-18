# Agent Instructions

Intelligent agents will reference this file once at the start of a session if given as context and follow the instructions:

1. Review #README.md, the README should describe the functionality of #codebase adequately, if there are _major_ differences between the functionality of codebase and the README.md make update to README, if there is no README, then create one describing the project.
2. Review #CONTRIBUTING.md, and then #codebase, including any lint rules or developmement patterns established. Update (if there are major differences) or create CONTRIBUTING with a development style guide and rules common to this file
3. For every commit or change you make to the file, before making the change make sure that it follows the rules in CONTRIBUTING document
4. If possible, alert user every time their input is needed or work is complete by providing two bells in your response (ctrl-g ctrl-g)