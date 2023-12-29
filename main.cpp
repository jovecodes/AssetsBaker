#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <variant>
#include <vector>

#define HEADER_COLOR "\033[95m"
#define OKBLUE_COLOR "\033[94m"
#define OKCYAN_COLOR "\033[96m"
#define OKGREEN_COLOR "\033[92m"
#define WARNING_COLOR "\033[93m"
#define FAIL_COLOR "\033[91m"
#define ENDC_COLOR "\033[0m"
#define BOLD_COLOR "\033[1m"
#define UNDERLINE_COLOR "\033[4m"

std::vector<std::string> files_in_dir(const std::string &dir) {
  DIR *dr;
  struct dirent *en;
  dr = opendir(dir.c_str()); // open all directory

  std::vector<std::string> files;

  if (dr) {
    while ((en = readdir(dr)) != NULL) {
      files.push_back(en->d_name);
    }
    if (closedir(dr) != 0) {
      std::cout
          << "ERROR: could not open current directory for some reason. :(\n";
    }
  }

  return files;
}

std::string get_file_extension(const std::string &filename) {
  size_t dotPos = filename.find_last_of('.');
  if (dotPos != std::string::npos && dotPos < filename.length() - 1) {
    return filename.substr(dotPos + 1);
  }
  return ""; // Empty string if no extension found
}

std::string read_entire_file(const std::string &filename) {
  std::ifstream t(filename);
  std::stringstream buffer;
  buffer << t.rdbuf();
  return buffer.str();
}

static bool write_entire_file(const std::string &contents,
                              const std::string &path) {
  std::ofstream file(path);

  if (!file.is_open()) {
    std::cerr << "Unable to open file '" << path << "'\n";
    return false;
  }

  file << contents;
  file.close();
  return true;
}

std::string write_hex_dump(const std::string &data, const std::string &path,
                           std::ofstream &file) {

  if (!file.is_open()) {
    std::cerr << "Unable to open file '" << path << "'\n";
    return "";
  }

  std::string path_for_name = path;
  for (size_t i = 0; i < path_for_name.length(); ++i) {
    if (path_for_name[i] == '.') {
      path_for_name[i] = '_';
    }
  }
  file << "static unsigned char " << path_for_name << "[] = {\n";

  size_t i = 0;

  for (; i < data.size(); ++i) {
    if (i % 16 == 0) {
      file << "    ";
    }
    file << "0x" << std::hex
         << static_cast<int>(static_cast<unsigned char>(data[i])) << ",";
    if (i % 16 == 15 || i == data.size() - 1) {
      file << "\n";
    } else {
      file << " ";
    }
  }

  file << "};\n";

  file << "static unsigned int " << path_for_name
       << "_len = " << std::to_string(i) << ";\n\n";
  return path_for_name;
}

std::string to_name(const std::string &file) {
  std::string name;
  for (char c : file) {
    if (c == '.')
      return name;
    name += c;
  }
  return name;
}

int main(void) {
  std::string result;
  std::string load_fn =
      "std::vector<TextureBank::NamedTexture> textures_to_load() { \n"
      "\treturn {\n";

  std::ofstream file("assets.h");

  file << "#pragma once\n\n";

  std::vector<std::string> files = files_in_dir(".");
  std::vector<std::string> fonts;

  bool in_assets = true;
  for (auto &f : files) {
    if (f == "assets") {
      in_assets = false;
    }
  }

  if (!in_assets) {
    files = files_in_dir("assets");
    fonts = files_in_dir("assets/fonts");
  } else {
    fonts = files_in_dir("fonts");
  }

  for (auto &f : files) {
    std::string extension = get_file_extension(f);
    if (extension == "png" || extension == "jpeg" || extension == "jpg") {
      std::cout << "Loading texture " << f << " ";
      std::string file_path = in_assets ? f : "assets/" + f;
      std::string file_contents = read_entire_file(file_path);
      std::string path_for_name = write_hex_dump(file_contents, f + ".h", file);

      load_fn += "\t\t{\"" + to_name(f) + "\", new Texture(" + path_for_name +
                 ", " + path_for_name + "_len)},\n";
      std::cout << OKGREEN_COLOR << "OK\n" << ENDC_COLOR;
    }
  }
  load_fn += "\t}; \n}\n\n";
  load_fn += "static std::vector<FontBank::NamedFont> fonts_to_load() {\n"
             "\t{\n";

  for (auto &f : fonts) {
    std::string extension = get_file_extension(f);
    if (extension == "ttf" || extension == "otf") {
      std::cout << "Loading font " << f << " ";
      std::string file_path = in_assets ? "fonts/" + f : "assets/fonts/" + f;
      std::string file_contents = read_entire_file(file_path);
      std::string path_for_name = write_hex_dump(file_contents, f + ".h", file);

      load_fn += "\t\t{\"" + to_name(f) + "\", new rendering::Font(" +
                 path_for_name + ", " + path_for_name + "_len), 32.0},\n";
      std::cout << OKGREEN_COLOR << "OK\n" << ENDC_COLOR;
    }
  }

  load_fn += "\t}; \n}\n";

  std::cout << result;

  file << "/*\n";
  file << load_fn;
  file << "*/\n";

  std::cout << "Beep boop. Done!\n\n";

  std::cout << load_fn;

  file.close();
  return 0;
}
