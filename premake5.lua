workspace "Gex3DViewer"
	location "build"
	configurations { "Debug", "Release" }
	language "C++"
	targetdir "bin"
	objdir "obj"
	systemversion "latest"
	cppdialect "c++latest"
	filter "system:windows"
		platforms "x86"
		characterset "MBCS"
	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}

project "Gex3DViewer"
	location "build"
	kind "ConsoleApp"
	debugdir "src"
	includedirs {
		"src",
		"include"
	}

	files {
		"src/**.c**",
		"src/**.h",
		"data/**.**",
		"premake5.lua",
		"data/**.**",
		"GenerateProject.bat"
	}

	links { 
		"opengl32",
		"glfw3dll"
	}

	filter "configurations:Debug"
		defines "DEBUG"