#include "node/parse.h"

#include <fstream>
#include <sstream>

#include "node/node.h"
#include "node/impl.h"
#include "parser.h"
#include "nodebuilder.h"

YAML_NAMESPACE_ENTER
Node Load(const std::string& input) {
  std::stringstream stream(input);
  return Load(stream);
}

Node Load(const char* input) {
  std::stringstream stream(input);
  return Load(stream);
}

Node Load(std::istream& input) {
  Parser parser(input);
  NodeBuilder builder;
  if (!parser.HandleNextDocument(builder))
    return Node();

  return builder.Root();
}

Node LoadFile(const std::string& filename) {
  std::ifstream fin(filename.c_str());
  if (!fin)
    throw BadFile();
  return Load(fin);
}

std::vector<Node> LoadAll(const std::string& input) {
  std::stringstream stream(input);
  return LoadAll(stream);
}

std::vector<Node> LoadAll(const char* input) {
  std::stringstream stream(input);
  return LoadAll(stream);
}

std::vector<Node> LoadAll(std::istream& input) {
  std::vector<Node> docs;

  Parser parser(input);
  while (1) {
    NodeBuilder builder;
    if (!parser.HandleNextDocument(builder))
      break;
    docs.push_back(builder.Root());
  }

  return docs;
}

std::vector<Node> LoadAllFromFile(const std::string& filename) {
  std::ifstream fin(filename.c_str());
  if (!fin)
    throw BadFile();
  return LoadAll(fin);
}
YAML_NAMESPACE_EXIT
