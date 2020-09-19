#include "json.h"
#include <cmath>

using namespace std;

namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }

  /*Node LoadDouble(istream& input) {
    double result_double;
    int minus = 1;
    if (input.peek() == '-') { 
      minus *= - 1;
      input.ignore();
    }
    uint64_t result = 0;

    while (isdigit(input.peek())) {
      result *= 10;
      result += input.get() - '0';
    }

    result *= minus;

    bool isDouble = false;

    if (input.peek() == '.') {
      input.ignore();
      result_double = result;
      isDouble = true;
      for(int i = 1; isdigit(input.peek()); i++) {
        result_double += ((input.get() - '0') / (std::pow(10, i))) * minus;
      }
    }
    return (isDouble) ? result_double : static_cast<double>(result);
  }*/

  Node LoadDouble(istream& input){
    double result;
    input >> result;
    return Node(result);
  }

  Node LoadBool(istream& input) {
    bool result = input.peek() == 't';
    if (result){
      input.ignore(4);
    }else{
      input.ignore(5);
    }

    return Node(result);
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else if (c == 't' || c == 'f'){
      input.putback(c);
      return LoadBool(input);
    } else {
      input.putback(c);
      return LoadDouble(input);
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }

}
