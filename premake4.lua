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

    files { "./source/**.hpp", "./source/**.def",
            "./source/**.cpp", "./source/**.tpp" }

    includedirs { "./source/" }
    vpaths { ["*"] = "./source/**" }

    links { "gecodeminimodel",
			"gecodedriver",
			"gecodegist",
			"gecodesearch",
			"gecodeset",
			"gecodefloat",
			"gecodeint",
			"gecodekernel",
			"gecodesupport",
			"stdc++" }
	buildoptions { "-std=c++11" }

  configuration "debug"
    defines { "DEBUG" }
    targetname "debug"

  configuration "dev"
    targetname "dev"

