from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.build import check_min_cppstd

class KlConan(ConanFile):
    name = "kl"
    version = "0.1"

    license = "MIT"
    url = "https://github.com/k0zmo/kl"
    package_type = "static-library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "fPIC": [True, False],
        "with_json": [True, False],
        "with_yaml": [True, False]
    }
    default_options = {
        "fPIC": True,
        "with_json": True,
        "with_yaml": True,
        "boost/*:header_only": True
    }
    exports_sources = "CMakeLists.txt", "klConfig.cmake.in", "cmake/*", "include/*", "src/*", "tests/*"
    no_copy_source = True

    def _build_and_run_tests(self):
        return not self.conf.get("tools.build:skip_test", default=False)

    def validate(self):
        check_min_cppstd(self, "17")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("ms-gsl/4.2.0", transitive_headers=True)
        self.requires("boost/1.86.0", transitive_headers=True)
        if self.options.with_json:
            self.requires("rapidjson/cci.20220822", transitive_headers=True)
        if self.options.with_yaml:
            self.requires("yaml-cpp/0.7.0", transitive_headers=True)

    def build_requirements(self):
        self.test_requires("catch2/3.2.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["KL_FETCH_DEPENDENCIES"] = False
        tc.cache_variables["KL_ENABLE_JSON"] = self.options.with_json
        tc.cache_variables["KL_ENABLE_YAML"] = self.options.with_yaml
        tc.cache_variables["KL_TEST"] = self._build_and_run_tests()
        # This is only used for "development workflow" but it kinda conflicts
        # with conan_provider.cmake that we use. It also confuses VS Code's
        # CMake tools as recreating CMakeUserPresets.json at the end of CMake's
        # configuration phase causes it to mark it as dirty and thus CMake
        # reconfigure every single time. NB: For package creation, CMake's
        # presets are not used - all parameters are passed through command line.
        tc.user_presets_path = None
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self._build_and_run_tests():
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        debug_suffix = 'd' if self.settings.build_type == 'Debug' else ''
        self.cpp_info.components["core"].libs = [f"kl{debug_suffix}"]
        self.cpp_info.components["core"].requires = ["ms-gsl::ms-gsl", "boost::headers"]
        self.cpp_info.components["core"].set_property("cmake_target_name", "kl::kl")
        if self.options.with_json:
            self.cpp_info.components["json"].requires = ["core", "rapidjson::rapidjson"]
            self.cpp_info.components["json"].libs = [f"kl-json{debug_suffix}"] 
        if self.options.with_yaml:
            self.cpp_info.components["yaml"].requires = ["core", "yaml-cpp::yaml-cpp", "ms-gsl::ms-gsl"]
            self.cpp_info.components["yaml"].libs = [f"kl-yaml{debug_suffix}"]
        self.cpp_info.set_property("cmake_target_name", "kl::all")
