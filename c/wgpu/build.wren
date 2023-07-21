// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

// dependencies

Meta.setenv("DEVSET", "aquabsd.black")
Deps.git_inherit("https://github.com/inobulles/aqua-unix")

Deps.git_inherit("https://github.com/inobulles/aqua-c")

// compilation

var cc = CC.new()

cc.add_opt("-Wall")
cc.add_opt("-Wextra")
cc.add_opt("-Werror")
cc.add_opt("-Wno-unused-function")
cc.add_opt("-fPIC")

var src = File.list("src")
	.where { |path| path.endsWith(".c") }

src
	.each { |path| cc.compile(path) }

// link program

var linker = Linker.new()
linker.link(src.toList, [], "main", true)

// install resources

Resources.install("res")

// running

class Runner {
	static pre_package { "zpk" }
	static run(args) { File.exec("kos", ["--boot", "default.zpk"]) }
}

// installation map

var entry = "wgpu"

var install = {
	"main": entry,
	"res": "res",
}

// packaging

var pkg = Package.new(entry)

pkg.unique = "examples.aqua.wgpu"
pkg.name = "WebGPU Example"
pkg.description = "Example of WebGPU usage on AQUA through the aquabsd.black.wgpu device."
pkg.version = "0.1.0"
pkg.author = "Aymeric Wibo"
pkg.organization = "Inobulles"
pkg.www = "https://github.com/inobulles/aqua-examples"

var packages = {
	"default": pkg,
}

// testing

class Tests {
}

var tests = []
