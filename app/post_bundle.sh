#!/bin/bash
# Script parameters
contents_dir=$1
macos_dir=$2
resources_dir=$3
plist_path=$4
prefix=$5

# Create the macOS app bundle directories, if not existing
mkdir -p "${macos_dir}"
mkdir -p "${resources_dir}"

cp "${MESON_BUILD_ROOT}/app/scrcpy" "${macos_dir}/scrcpy"
cp "${plist_path}" "${contents_dir}/Info.plist"
cp $(which adb) $macos_dir
lipo -thin arm64 $macos_dir/adb -o $macos_dir/adb

# Copy and convert the PNG icon to ICNS
icon_name="scrcpy.icns"
png_path="${resources_dir}/scrcpy.png"  # Path to your PNG file
iconset_path="${resources_dir}/icon.iconset"

# Create a temporary iconset directory
mkdir -p "${iconset_path}"

# Generate multiple sizes for ICNS
sips -z 16 16   "${png_path}" --out "${iconset_path}/icon_16x16.png"
sips -z 32 32   "${png_path}" --out "${iconset_path}/icon_16x16@2x.png"
sips -z 32 32   "${png_path}" --out "${iconset_path}/icon_32x32.png"
sips -z 64 64   "${png_path}" --out "${iconset_path}/icon_32x32@2x.png"
sips -z 128 128 "${png_path}" --out "${iconset_path}/icon_128x128.png"
sips -z 256 256 "${png_path}" --out "${iconset_path}/icon_128x128@2x.png"
sips -z 256 256 "${png_path}" --out "${iconset_path}/icon_256x256.png"
sips -z 512 512 "${png_path}" --out "${iconset_path}/icon_256x256@2x.png"
sips -z 512 512 "${png_path}" --out "${iconset_path}/icon_512x512.png"
sips -z 1024 1024 "${png_path}" --out "${iconset_path}/icon_512x512@2x.png"

iconutil -c icns "${iconset_path}" -o "${resources_dir}/${icon_name}"

rm -r "${iconset_path}"

chmod +x "${macos_dir}/scrcpy"

echo "APPL????" > "${contents_dir}/PkgInfo"

chmod 644 "${contents_dir}/PkgInfo"

rm -rf $prefix/bin

APP_EXEC="$macos_dir/scrcpy"
FRAMEWORKS_DIR="$contents_dir/Frameworks"

mkdir -p "${FRAMEWORKS_DIR}"

mv $APP_EXEC "$macos_dir/scrcpy-macos"
echo '#!/bin/bash' > "$macos_dir/scrcpy"
echo 'macos=$(dirname $0); contents=$(dirname $macos)' >> "$macos_dir/scrcpy"
echo 'export SCRCPY_SERVER_PATH=$contents/resources/scrcpy-server' >> "$macos_dir/scrcpy"
echo '$(dirname $0)/scrcpy-macos --turn-screen-off --video-encoder "OMX.google.h264.encoder" --max-fps 30' >> "$macos_dir/scrcpy"
chmod +x "${macos_dir}/scrcpy"