#ifndef MY_LIB_HPP
#define MY_LIB_HPP

#include <string>
#include <memory>

class Lib {

public:
	Lib(const std::string& description, double version) : description(description), version(version) {}
	~Lib() {}

	std::string get_description() const { return description; }
	double get_version() const { return version; }
	std::string to_str() const { return description + " - " + std::to_string(version); }

	void set_description(const std::string& s) { description = s; }
	void set_version(double v) { version = v; }

	static std::shared_ptr<Lib> get_instance(const std::string& s, double v) {
		return std::make_shared<Lib>(s, v);
	}
private:
	std::string description;
	double version;
};


#endif // MY_LIB_HPP