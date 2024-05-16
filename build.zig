const std = @import("std");
const builtin = std.builtin;

const Build = std.Build;
const ResolvedTarget = Build.ResolvedTarget;
const OptimizeMode = builtin.OptimizeMode;

const flags = [_][]const u8{ "-Wall", "-Wextra" };
const include_path = "./include";
const library_path = "./lib";

const Program = struct {
    name: []const u8,
    libraries: []const []const u8,
    frameworks: []const []const u8 = &.{},
};

const programs = [5]Program{
    .{
        .name = "main",
        .libraries = &.{ "avcodec", "avformat", "avutil", "glfw3", "iconv", "z" },
        .frameworks = &.{ "OpenGL", "Cocoa", "IOKit", "CoreFoundation" },
    },
    .{
        .name = "encode_video_x264",
        .libraries = &.{"x264"},
    },
    .{
        .name = "encode_video_x265",
        .libraries = &.{ "x265", "c++" },
    },
    .{
        .name = "decode_video",
        .libraries = &.{ "avcodec", "avutil", "iconv" },
    },
    .{
        .name = "decode_audio",
        .libraries = &.{ "avcodec", "avutil", "iconv" },
    },
};

pub fn build(b: *Build) void {
    const target: ResolvedTarget = b.host;
    const optimize: OptimizeMode = .ReleaseFast;

    inline for (programs) |program| {
        const exe = b.addExecutable(.{
            .name = program.name,
            .target = target,
            .optimize = optimize,
            .strip = true,
            .pic = true,
        });

        exe.addIncludePath(b.path(include_path));
        exe.addLibraryPath(b.path(library_path));

        for (program.frameworks) |framework| {
            exe.linkFramework(framework);
        }

        for (program.libraries) |library| {
            exe.linkSystemLibrary(library);
        }

        const sources = [_][]const u8{ program.name ++ ".c", "util.c" };

        exe.addCSourceFiles(.{
            .root = b.path("src"),
            .files = &sources,
            .flags = &flags,
        });

        b.installArtifact(exe);
    }
}
