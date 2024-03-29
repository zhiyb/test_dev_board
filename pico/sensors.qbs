import qbs

Project {
    minimumQbsVersion: "1.21.0"

    property bool pico_double: true
    property bool pico_float: true
    property bool pico_printf: true

    property stringList pico_sdk_defines: [
        "NDEBUG",
        "PICO_PANIC_FUNCTION=",
        "PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=0",
    ]

    references: [
        "pico-sdk.qbs",
    ]

    StaticLibrary {
        name: "adc"
        Depends {name: "pico-sdk"}

        cpp.includePaths: ["inc"]

        files: [
            "inc/adc.h",
            "src/adc.c",
        ]
    }

    StaticLibrary {
        name: "bmp2"
        Depends {name: "pico-sdk"}

        cpp.includePaths: ["inc", "bmp2"]

        files: [
            "bmp2/bmp2_defs.h",
            "bmp2/bmp2.h",
            "bmp2/bmp2.c",
            "inc/sensor_bmp2.h",
            "src/sensor_bmp2.c",
        ]
    }

    StaticLibrary {
        name: "bme68x"
        Depends {name: "pico-sdk"}

        cpp.includePaths: ["inc", "bme68x"]

        files: [
            "bme68x/bme68x_defs.h",
            "bme68x/bme68x.h",
            "bme68x/bme68x.c",
            "inc/sensor_bme68x.h",
            "src/sensor_bme68x.c",
        ]
    }

    StaticLibrary {
        name: "bh1745"
        Depends {name: "pico-sdk"}

        cpp.includePaths: ["inc"]

        files: [
            "inc/sensor_bh1745.h",
            "src/sensor_bh1745.c",
        ]
    }

    CppApplication {
        type: ["application", "hex", "size", "map"]
        Depends {name: "gcc-none"}
        Depends {name: "pico-sdk"}
        Depends {name: "adc"}
        Depends {name: "bmp2"}
        Depends {name: "bme68x"}
        Depends {name: "bh1745"}

        cpp.includePaths: ["inc"]

        files: [
            "src/main.c",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: ["application", "hex", "bin", "map"]
            qbs.install: true
        }
    }
}
