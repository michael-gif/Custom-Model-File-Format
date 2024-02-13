# Custom-Model-File-Format
Specialised file format for storing mesh data. (Vertices, triangles, uvs and normals)

### Requirements
- You need autodesks `libfbxsdk.dll` when running the exe. I don't know which
version, so i included it in the files under the `autodesk fbx dll\` directory.
`libfbxsdk.dll` is also included in the [latest release](https://github.com/michael-gif/Custom-Model-File-Format/releases/tag/1.0.0)
- The program checks the first mesh of the first object in the fbx file, so to
avoid issues, only have one object with one mesh in the file.

### Usage
Syntax:  
`modelmaker <input fbx file> <output file with any extension>`  
Example:  
`modelmaker model.fbx model.m`

### Compiling
When compiling, make sure u have the following include path  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\include`  
And the library path  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\lib\vs2017\x64\release`  
And additional dependencies  
`libfbxsdk.lib`