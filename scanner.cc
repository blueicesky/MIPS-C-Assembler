#include "scanner.h"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>



namespace CS241 {
Token::Token(std::string kind, std::string lexeme):kind(kind), lexeme(lexeme) {}

std::string Token::getKind() const { return kind; }
std::string Token::getLexeme() const { return lexeme; }

long long Token::toLong() const {
  std::istringstream iss;
  long long result;

  if (kind == "INT") {
    iss.str(lexeme);
  } else if (kind == "HEXINT") {
    iss.str(lexeme.substr(2));
    iss >> std::hex;
  } else if (kind == "REG") {
    iss.str(lexeme.substr(1));
  } else {
    // This should never happen if the user calls this function correctly
    return 0;
  }

  iss >> result;
  return result;
}

int Token::toInt() const {
  if (kind == "REG") {
    return (int)toLong();
  } else {
    // This should never happen if the user calls this function correctly
    return 0;
  }
}

ScanningFailure::ScanningFailure(std::string message):message(message) {}

std::string ScanningFailure::getMessage() const { return message; }

AsmDFA::AsmDFA() {
  acceptingStates = {"ID", "LABEL", "DOTID", "HEXINT",
                     "INT", "ZERO", "COMMA", "REG",
                     "LPAREN", "RPAREN", "WHITESPACE", "COMMENT"};
  //Non-accepting states are "DOT", "MINUS", "ZEROX", "DOLLARS", "START"
}

// The transition function for the DFA. Could be implemented
// using a map or something more fancy, but this is implemented using nested
// if statements for simplicity.
std::string AsmDFA::transition(std::string state, char nextChar) const {
  if (state == "START") {
    if (isalpha(nextChar))      return "ID";
    else if (nextChar == '.')   return "DOT";
    else if (nextChar == '0')   return "ZERO";
    else if (isdigit(nextChar)) return "INT";
    else if (nextChar == '-')   return "MINUS";
    else if (nextChar == ';')   return "COMMENT";
    else if (isspace(nextChar)) return "WHITESPACE";
    else if (nextChar == '$')   return "DOLLARS";
    else if (nextChar == ',')   return "COMMA";
    else if (nextChar == '(')   return "LPAREN";
    else if (nextChar == ')')   return "RPAREN";
  } else if (state == "ID") {
    if (isalnum(nextChar))      return "ID";
    else if (nextChar == ':')   return "LABEL";
  } else if (state == "DOT" || state == "DOTID") {
    if (isalnum(nextChar))      return "DOTID";
  } else if (state == "ZERO") {
    if (nextChar == 'x')        return "ZEROX";
    else if (isdigit(nextChar)) return "INT";
  } else if (state == "ZEROX" || state == "HEXINT") {
    if (isxdigit(nextChar))     return "HEXINT";
  } else if (state == "INT" || state == "MINUS") {
    if (isdigit(nextChar))      return "INT";
  } else if (state == "COMMENT") {
    if (nextChar != '\n')       return "COMMENT";
  } else if (state == "WHITESPACE") {
    if (isspace(nextChar))      return "WHITESPACE";
  } else if (state == "DOLLARS" || state == "REG") {
    if (isdigit(nextChar))      return "REG";
  }

  return "FAIL";
}

bool AsmDFA::failed(std::string state) const {
  return state == "FAIL";
}

bool AsmDFA::accept(std::string state) const {
  return acceptingStates.count(state) > 0;
}

std::string AsmDFA::start() const {
  return "START";
}

std::vector<Token> AsmDFA::simplifiedMaximalMunch(std::string input) const {
  std::vector<Token> result;

  std::string state = start();
  std::string munched_input;

  // We can't use a range-based for loop effectively here
  // since the iterator doesn't always increment.
  for (std::string::iterator input_posn = input.begin();
       input_posn != input.end();) {

    std::string old_state = state;
    state = transition(state, *input_posn);

    if (!failed(state)) {
      munched_input += *input_posn;
      old_state = state;

      ++input_posn;
    }

    if (input_posn == input.end() || failed(state)) {
      if (accept(old_state)) {
        result.push_back(Token(old_state, munched_input));

        munched_input = "";
        state = start();
      } else {
        if (failed(state)) {
          munched_input += *input_posn;
        }
        throw ScanningFailure("ERROR: Simplified maximal munch failed on input: "
                             + munched_input);
      }
    }

  }

  return result;
}

std::vector<Token> AsmDFA::scan(std::string input) const {
  std::vector<Token> tokens = simplifiedMaximalMunch(input);

  // We need to:
  // * Convert "ZERO" tokens to "INT" tokens.
  // * Convert "DOTID" tokens to "WORD" tokens if they're ".word",
  //   and throw an exception otherwise.
  // * Verify that INTs, HEXINTs, and REGs occupy the proper ranges:
  //   * -2^31 to 2^32-1 for INT (both signed and unsigned INT are supported)
  //   * 0 to 2^32-1 for HEXINT
  //   * 0 to 31 for REG
  // * Remove WHITESPACE and COMMENT tokens entirely.

  std::vector<Token> newTokens;

  for (auto &token : tokens) {
    if (token.getKind() == "ZERO") {
      newTokens.push_back(Token("INT",token.getLexeme()));
    } else if (token.getKind() == "DOTID") {
      if (token.getLexeme() == ".word") {
        newTokens.push_back(Token("WORD",token.getLexeme()));
      } else {
        throw ScanningFailure("ERROR: DOTID token unrecognized: " + token.getLexeme());
      }
    } else if (token.getKind() == "REG" || token.getKind() == "INT"
            || token.getKind() == "HEXINT") {
      long long lower_bound = token.getKind() == "INT" ? -2147483648ll : 0;
      long long upper_bound = token.getKind() == "REG" ? 31 : 4294967295ll;

      if (token.toLong() >= lower_bound && token.toLong() <= upper_bound) {
        newTokens.push_back(token);
      } else {
        throw ScanningFailure("ERROR: " + token.getKind() + " token out of range: "
                             + token.getLexeme());
      }
    } else if (token.getKind() != "WHITESPACE" && token.getKind() != "COMMENT") {
      newTokens.push_back(token);
    }
  }

  return newTokens;
}
}
