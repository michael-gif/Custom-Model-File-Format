# Custom-Model-File-Format
Specialised file format for storing mesh data. (Vertices, triangles, uvs and normals)

You need autodesks libfbxsdk.dll when running the exe. I don't know which version, so i included it in the files under the 'autodesk fbx dll' directory.

When compiling, make sure u have the following include path  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\include`  
And the library path  
`C:\Program Files\Autodesk\FBX\FBX SDK\2020.0.1\lib\vs2017\x64\release`  
And additional dependencies  
`libfbxsdk.lib`