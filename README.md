# Custom-Model-File-Format
Specialised file format for storing mesh data. (Vertices, triangles, uvs and normals)

### Requirements for running
Visual C++ runtime dlls:
- `msvcp140.dll`, `vcruntime140.dll`, `vcruntime140_1.dll`  

Autodesk FBX SDK dll:
- `libfbxsdk.dll`  

All dependencies are included in the [latest release](https://github.com/michael-gif/Custom-Model-File-Format/releases/tag/1.0.0)
as well as the [`dependencies\`](https://github.com/michael-gif/Custom-Model-File-Format/tree/main/dependencies) directory of this repository
- The program checks the first mesh of the first object in the fbx file, so to
avoid issues, only have one object with one mesh in the file.

### Usage
Syntax:  
`modelmaker <input fbx file> <output file with any extension>`  
Example:  
`modelmaker model.fbx model.m`

### Compiling from source
- When compiling, make sure u have the following include path:  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\include`  
- And the library path:  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\lib\vs2017\x64\release`  
- And additional dependencies:  
`libfbxsdk.lib`  
- Use /MD when compiling with MSVC, and paste the runtime dlls in the same folder
as the compiled exe:
```
dependencies\libfbxsdk.dll
dependencies\msvcp140.dll
dependencies\vcruntime140.dll
dependencies\vcruntime140_1.dll
```