cd $PSScriptRoot

if (!$env:SCOOP){
    $env:SCOOP = Join-Path $env:USERPROFILE Scoop
}

$nasm = (Get-command nasm -ErrorAction Stop).Source
$gcc = (Get-Command -ErrorAction Stop gcc).Source

# you MUST have a SHARED build (.h, .lib and .dll files)
$ffmpeg_shared_path = "$env:SCOOP\apps\ffmpeg-shared\current"

if (-not(Test-Path "$ffmpeg_shared_path\include\libavcodec\avcodec.h")){
    return "`nYour ffmpeg installation at ``$ffmpeg_shared_path`` is either not a Shared edition, incomplete or does not exist at this path"
}

$out = "./build/out/video-blender.exe"
$out_dir = $out | Split-Path
if (-not(Test-Path $out_dir)){
    mkdir $out_dir
}
if (Test-Path $out){
    Remove-Item $out -ErrorAction Stop
}

& $nasm -f win64 .\src\blendlib\blendingx86.asm

& $gcc -std=c99 -Ofast `
.\src\blendlib\blending.c `
.\src\blendlib\blendlib.c `
.\src\blendlib\convert.c `
.\src\blendlib\RGBFrame.c `
.\src\utils\Error.c `
.\src\utils\Mutex.c `
.\src\utils\Mutex.h `
.\src\utils\Queue.c `
.\src\utils\Queue.h `
.\src\utils\Stock.c `
.\src\utils\Stock.h `
.\src\utils\Thread.c `
.\src\utils\Thread.h `
.\src\utils\Types.h `
.\src\utils\Utils.c `
.\src\utils\Utils.h `
.\src\vblender\Coding.c `
.\src\vblender\Coding.h `
.\src\vblender\vblender.c `
.\src\vblender\vblender.h `
.\src\main.c `
.\src\blendlib\blendingx86.obj `
-I"$(Join-Path $ffmpeg_shared_path include)" `
-L"$(Join-Path $ffmpeg_shared_path lib)" `
-lavformat -lavcodec -lswscale -lavutil `
-I".\src" `
-o"$($out)"

if (-not(Test-Path "$out_dir/*.dll")){
    $dependencies = @(
        "avutil-*.dll"
        "swresample-*.dll"
        "swscale-*.dll"
        "avcodec-*.dll"
        "avformat-*.dll"
        ) | ForEach-Object { "$ffmpeg_shared_path\bin\$PSitem" }
        
        Copy-Item $dependencies $out_dir
}
