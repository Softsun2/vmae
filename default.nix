{ stdenv, cmake, pkg-config, ffmpeg_6 }:
stdenv.mkDerivation {
  pname = "vmae";
  version = "1.0";
  src = ./.;
  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ ffmpeg_6 ];
}
