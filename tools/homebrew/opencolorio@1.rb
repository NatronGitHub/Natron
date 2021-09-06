class OpencolorioAT1 < Formula
  desc "Color management solution geared towards motion picture production"
  homepage "https://opencolorio.org/"
  url "https://github.com/imageworks/OpenColorIO/archive/v1.1.1.tar.gz"
  sha256 "c9b5b9def907e1dafb29e37336b702fff22cc6306d445a13b1621b8a754c14c8"
  license "BSD-3-Clause"
  revision 3
  head "https://github.com/imageworks/OpenColorIO.git"

  bottle do
  end

  keg_only :versioned_formula

  depends_on "cmake" => :build
  depends_on "pkg-config" => :build
  depends_on "little-cms2"
  depends_on "python@3.9"

  def install
    args = std_cmake_args + %W[
      -DCMAKE_VERBOSE_MAKEFILE=OFF
      -DCMAKE_INSTALL_RPATH=#{rpath}
      -DPYTHON=python3
      -DPYTHON_EXECUTABLE=#{Formula["python@3.9"].opt_bin}/"python3"
    ]

    mkdir "macbuild" do
      system "cmake", *args, ".."
      system "make"
      system "make", "install"
    end
  end

  def caveats
    <<~EOS
      OpenColorIO requires several environment variables to be set.
      You can source the following script in your shell-startup to do that:
        #{HOMEBREW_PREFIX}/share/ocio/setup_ocio.sh

      Alternatively the documentation describes what env-variables need set:
        https://opencolorio.org/installation.html#environment-variables

      You will require a config for OCIO to be useful. Sample configuration files
      and reference images can be found at:
        https://opencolorio.org/downloads.html
    EOS
  end

  test do
    assert_match "validate", shell_output("#{bin}/ociocheck --help", 1)
  end
end
