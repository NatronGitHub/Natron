class SeexprAT2 < Formula
  desc "Embeddable expression evaluation engine"
  homepage "https://www.disneyanimation.com/technology/seexpr.html"
  url "https://github.com/wdas/SeExpr/archive/v2.11.tar.gz"
  sha256 "bf4a498f86aa3fc19aad3d7384de11d5df76f7f71587c9bd789f5e50f8090e1a"

  bottle do
  end

  keg_only :versioned_formula

  depends_on "cmake" => :build
  depends_on "doxygen" => :build
  depends_on "libpng"

  def install
    mkdir "build" do
      # must probably "brew unlink qt@4" before building
      system "cmake", "..", *std_cmake_args, "-DQT4_FOUND:BOOL=False"
      system "make", "doc"
      system "make", "install"
    end
  end

  test do
    system bin/"asciigraph"
  end
end
