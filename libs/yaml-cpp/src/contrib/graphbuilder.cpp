#include "graphbuilderadapter.h"

#include "parser.h"  // IWYU pragma: keep

YAML_NAMESPACE_ENTER
class GraphBuilderInterface;

void* BuildGraphOfNextDocument(Parser& parser,
                               GraphBuilderInterface& graphBuilder) {
  GraphBuilderAdapter eventHandler(graphBuilder);
  if (parser.HandleNextDocument(eventHandler)) {
    return eventHandler.RootNode();
  } else {
    return NULL;
  }
}
YAML_NAMESPACE_EXIT
