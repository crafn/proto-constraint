solution "gecode_test"
  configurations { "debug", "release" }
  platforms { "native" }

  configuration "debug"
    flags { "Symbols" }
    targetdir "debug"

  configuration "dev"
    flags { "Symbols", "Optimize" }
    targetdir "dev"

  project "app"
	kind "ConsoleApp"
    language "C++"

    files { "./eq/**.hpp", "./eq/**.def",
            "./eq/**.cpp", "./eq/**.tpp" }

    includedirs { "./eq/", "./deps/or-tools/include/" }
	libdirs { "./deps/or-tools/lib/" }
    vpaths { ["*"] = "./eq/**" }

    links { "ortools",
			"stdc++" }
	buildoptions { "-std=c++11" }

  configuration "debug"
    defines { "DEBUG" }
    targetname "debug"

  configuration "dev"
    targetname "dev"

