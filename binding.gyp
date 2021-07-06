{
    "targets": [
        {
            "target_name": "ba2tk",
            "includes": [
                "auto.gypi"
            ],
            "sources": [
                "ba2tk/src/ba2archive.cpp",
                "ba2tk/src/ba2exception.cpp",
                "string_cast.cpp",
                "index.cpp"
            ],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")",
                "./ba2tk/src/common",
                "./zlib/include"
            ],
            "dependencies": [
              "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "conditions": [
                [
                    'OS=="win"',
                    {
                        "defines!": [
                            "_HAS_EXCEPTIONS=0"
                        ],
                        "libraries": [
                            "-l../zlib/win32/zlibstatic.lib",
                            "-DelayLoad:node.exe"
                        ],
                        "msvs_settings": {
                            "VCCLCompilerTool": {
                                "ExceptionHandling": 1
                            }
                        },
                        "msbuild_settings": {
                          "ClCompile": {
                            "AdditionalOptions": ['-std:c++17']
                          }
                        }
                    }
                ],
                [
                    'OS=="mac"',
                    {
                        "xcode_settings": {
                            "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
                        }
                    }
                ]
            ]
        }
    ],
    "includes": [
        "auto-top.gypi"
    ]
}
