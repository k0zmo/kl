from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.build import check_min_cppstd

class KlConan(ConanFile):
    name = "kl"
    version = "0.1"

    license = "MIT"
    url = "https://github.com/k0zmo/kl"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "fPIC": [True, False],
        "with_json": [True, False],
        "with_yaml": [True, False],
        "build_tests": [True, False]
    }
    default_options = {
        "fPIC": True,
        "with_json": True,
        "with_yaml": True,
        "build_tests": False
    }
    exports_sources = "CMakeLists.txt", "klConfig.cmake.in", "cmake/*", "include/*", "src/*", "tests/*"
    no_copy_source = True

    def validate(self):
        check_min_cppstd(self, "17")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def build_requirements(self):
        if self.options.build_tests:
            self.test_requires("catch2/3.2.1")

    def requirements(self):
        self.requires("ms-gsl/4.0.0")
        self.requires("boost/1.81.0")
        if self.options.with_json:
            self.requires("rapidjson/cci.20220822")
        if self.options.with_yaml:
            self.requires("yaml-cpp/0.7.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["KL_TEST"] = self.options.build_tests
        tc.cache_variables["KL_FETCH_DEPENDENCIES"] = False
        tc.cache_variables["KL_ENABLE_JSON"] = self.options.with_json
        tc.cache_variables["KL_ENABLE_YAML"] = self.options.with_yaml
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.build_tests:
            cmake.test()
            
    def package(self):
        cmake = CMake(self)
        cmake.install()
        
    def package_id(self):
        self.info.options.rm_safe("build_tests")
        
    def package_info(self):
        self.cpp_info.components["_core"].libs = ["kl"]
        self.cpp_info.components["_core"].requires = ["ms-gsl::ms-gsl", "boost::headers"]
        if self.options.with_json:
            self.cpp_info.components["json"].requires = ["_core", "rapidjson::rapidjson"]
            self.cpp_info.components["json"].libs = ["kl-json"]
        if self.options.with_yaml:
            self.cpp_info.components["yaml"].requires = ["_core", "yaml-cpp::yaml-cpp"]
            self.cpp_info.components["yaml"].libs = ["kl-yaml"]
