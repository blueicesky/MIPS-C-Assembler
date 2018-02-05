#ifndef SCANNER_H
#define SCANNER_H
#include <string>
#include <vector>
#include <set>
#include <cstdint>



namespace CS241 {
/* A scanned token produced by the scanner.
 * The "kind" tells us what kind of token it is
 * (see the AsmDFA::scan() function for a full list),
 * while the "lexeme" tells us exactly what text
 * the programmer typed. For example, the token
 * "abc" might have kind "ID" and lexeme "abc".
 *
 * While you can create tokens with any kind and
 * lexeme, the list of kinds produced by the
 * starter code can be found in the documentation
 * for AsmDFA::scan below.
 */
class Token {
  private:
    /* It would be more efficient to manage kinds as
     * an enum, but we use strings for simplicity.
     */
    std::string kind;
    std::string lexeme;

  public:
    Token(std::string kind, std::string lexeme);

    std::string getKind() const;
    std::string getLexeme() const;

    /* May crash if called on tokens of type other than
     * INT, HEXINT, or REG.
     * 
     * Although all results will be integers, this function
     * returns a long since the result may be either signed
     * or unsigned, and thus may lie anywhere in the range
     * -2147483648 .. 4294967295
     */
    long long toLong() const;
    
    /* Only works on tokens of type REG
     * Does the same thing as toLong above, but returns type int
     * since REG values are always in the range 0 .. 31.
     * While we could use a smaller type such as short or char,
     * we use int for simplicity.
     */
    int toInt() const;
};

/* An exception class thrown when the AsmDFA class
 * encounters an error while scanning.
 */
class ScanningFailure {
  std::string message;
  public:
    ScanningFailure(std::string message);

    std::string getMessage() const;
};

/* Represents a DFA (which you will see formally in class
 * around the time of Assignment 5) to handle the scanning
 * process. You should not need to interact with this directly:
 * it is handled through the starter code.
 */
class AsmDFA {
  private:
    /* A set of all accepting states for the DFA.
     * Currently non-accepting states are not actually present anywhere
     * in memory, but a list can be found in scanner.cc if you're interested.
     */
    std::set<std::string> acceptingStates;

    /* Tokenizes an input string according to the SMM algorithm.
     * You will learn the SMM algorithm in class around the time of Assignment 6.
     */
    std::vector<Token> simplifiedMaximalMunch(std::string input) const;

  public:
    /* Initializes the accepting states for the DFA.
     */
    AsmDFA();

    /* Returns the state corresponding to following a transition
     * from the given starting state on the given character,
     * or a special fail state if the transition does not exist.
     */
    std::string transition(std::string state, char nextChar) const;

    /* Checks whether the state returned by transition
     * corresponds to failure to transition.
     */
    bool failed(std::string state) const;

    /* Checks whether the state returned by transition
     * is an accepting state.
     */
    bool accept(std::string state) const;

    /* Returns the starting state of the DFA
     */
    std::string start() const;

    /* Scans some input (can be the entire file or just one line)
     * and produces the corresponding list of tokens.
     * 
     * Scan returns tokens of the following kinds:
     * ID: Identifiers and keywords.
     * LABEL: Labels (identifiers ending in a colon).
     * WORD: The special ".word" keyword.
     * COMMA: A comma.
     * LPAREN: A left parenthesis.
     * RPAREN: A right parenthesis.
     * INT: A signed or unsigned 32-bit integer written in decimal.
     * HEXINT: An unsigned 32-bit integer written in hexadecimal.
     * REG: A register between $0 and $31.
     */
    std::vector<Token> scan(std::string input) const;
};
}

#endif
