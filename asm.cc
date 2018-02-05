#include "scanner.h"
#include <iostream>
#include <map>
#include <stdio.h>
#include <ctype.h>
#include <bitset>
using CS241::Token;
using namespace std;

// You can throw this class to signal exceptions encountered while assembling
class AssemblingFailure {
  	string message;
  	public:
		AssemblingFailure(string message):message(message) {}
		string getMessage() const { return message; }
};


void putword(int i){
  	putchar((i >> 24) & 0xff);
  	putchar((i >> 16) & 0xff);
  	putchar((i >> 8) & 0xff);
  	putchar((i) & 0xff);
}

bool validLabel(string label){
	if(isdigit(label[0])){
		return false;
	}else if(label[label.length() - 1] != ':'){
		return false;
	}else{
		if(label[label.length() - 1] == label[label.length() - 2]){
			return false;
		}
	}
	//implement check for 2 or more :'s
	return true;
}

int setUpjr(int reg,bool isjalr){
	int jr_temp = 8;
	if(isjalr){
		jr_temp = 9;
	}
	int shiftreg = reg << 21;
	return shiftreg | jr_temp;
}

int setUpAddSub(int regD, int regS, int regT, bool isSub){
	int temp = 32;
	if(isSub){
		temp = 34;
	}
	int shiftS = regS << 21;
	int shiftT = regT << 16;
	int shiftD = regD << 11;
	return shiftS | shiftT | shiftD | temp;
}

int setUpMult(int regS, int regT, int whichChoice){
	int temp = 24;
	if (whichChoice == 1){
		temp = 25;
	}else if(whichChoice == 2){
		temp = 26;
	}else if(whichChoice == 3){
		temp = 27;
	}
	int shiftS = regS << 21;
	int shiftT = regT << 16;
	return shiftS | shiftT | temp;
}

int setUpBne(int regS, int regT, int branch, bool isNotEqual){
	if(branch < 0){
		int abc = 65535;
		branch = branch & abc;
	}
	int temp = 268435456;
	if(isNotEqual){
		temp = 335544320;
	}
	int shiftS = regS << 21;
	int shiftT = regT << 16;
	return shiftT | shiftS | branch | temp;
}

int setUpSW(int regS, int regT, int lwInt, bool lword){
	if(lwInt < 0){
		int abc = 65535;
		lwInt = lwInt & abc;
	}
	int temp = 2885681152;
	if(lword){
		temp = 2348810240;
	}
	int shiftS = regS << 21;
	int shiftT = regT << 16;
	return shiftT | shiftS | lwInt | temp;
}

int setUplis(int regD, int whichChoice){
	//1 is lis
	//2 is mfhi
	//3 is mflo
	int temp = 16;
	if (whichChoice == 1){
		temp = 20;
	}else if(whichChoice == 3){
		temp = 18;
	}
	int shiftD = regD << 11;
	return shiftD | temp;
}

int setUpslt(int regD, int regS, int regT, bool isUnsigned){
	int temp = 42;
	if(isUnsigned){
		temp = 43;
	}
	int shiftS = regS << 21;
	int shiftT = regT << 16;
	int shiftD = regD << 11;
	return shiftS | shiftT | shiftD | temp;
}



pair<bool,map<string,int>> constructSymbolTable(vector<vector<Token>> &tokenLines){
	map<string,int> st;
	bool failedLine = false;
	int lineCounter = 0;
	for (auto &line : tokenLines) {
	// For now just print out the tokens, one line at a time.
	// You will probably want to replace this with your code.
		Token prev = Token("PLACEHOLDER","0x00000000");
		bool first = true;
		bool executedLoop = false;
		bool wordOccurred = false;
		bool added = false;
		bool labeledLine = false;
		bool onlyLabel = false;
		bool jrjalr = false;
		bool addSub = false;
		bool unsignedSlt = false;
		bool slt = false;
		bool subtract = false;
		int numRegs = 0;
		int reg1 = -1;
		int reg2 = -1;
		int reg3 = -1;
		int printThis = 0;

		//beq stuff
		bool beq = false;
		bool bne = false;
		int bneNum = 0;

		//lis mfhi mflo stuff
		bool lis = false;
		bool mfhi = false;
		bool mflo = false;
		bool gotReg = false;

		//mult div stuff
		bool mult = false;
		bool divi = false;
		bool multu = false;
		bool diviu = false;

		//sw
		bool storeword = false;
		bool loadword = false;
		int loadwordInt = 0;
		bool numberOcc = false;

		for (auto &token : line) {
			executedLoop = true;
			//cerr << "Token(" << token.getKind() << "," << token.getLexeme() << ") ";
			if(first){
				if(token.getKind() == "WORD"){
					wordOccurred = true;
				}else if(token.getKind() == "LABEL"){
					labeledLine = true;
					onlyLabel = true;
					if(validLabel(token.getLexeme())){
						string s = token.getLexeme();
						s.pop_back();
						if(st.count(s) != 0){//(st.find(token.getLexeme().pop_back()) != st.end()){
							cerr << "ERROR : Duplicate definition of a label" << endl;
							failedLine = true;
							break;
						}else{
							st.insert(pair<string,int>(s,lineCounter * 4));//[token.getLexeme().pop_back()] = lineCounter * 4;
						}
					}else{
						cerr << "ERROR : Not a valid label" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "ID"){
					if(token.getLexeme() == "jr" || token.getLexeme() == "jalr"){
						jrjalr = true;
					}else if(token.getLexeme() == "add" || token.getLexeme() == "sub"){
						addSub = true;
					}else if(token.getLexeme() == "slt" || token.getLexeme() == "sltu"){
						slt = true;
					}else if(token.getLexeme() == "bne" || token.getLexeme() == "beq"){
						if(token.getLexeme() == "bne"){
							bne = true;
							beq = false;
						}else{
							beq = true;
							bne = false;
						}
					}else if(token.getLexeme() == "lis" || token.getLexeme() == "mfhi" || token.getLexeme() == "mflo"){
						if(token.getLexeme() == "lis"){
							lis = true;
						}else if(token.getLexeme() == "mfhi"){
							mfhi = true;
						}else if(token.getLexeme() == "mflo"){
							mflo = true;
						}
					}else if(token.getLexeme() == "mult" || token.getLexeme() == "div" || token.getLexeme() == "divu" || token.getLexeme() == "multu"){
						if(token.getLexeme() == "divu"){
							diviu = true;
						}else if(token.getLexeme() == "div"){
							divi = true;
						}else if(token.getLexeme() == "mult"){
							mult = true;
						}else if(token.getLexeme() == "multu"){
							multu = true;
						}
					}else if(token.getLexeme() == "sw" || token.getLexeme() == "lw"){
						if(token.getLexeme() == "sw"){
							storeword = true;
						}else if(token.getLexeme() == "lw"){
							loadword = true;
						}
					}else{
						cerr << "ERROR : Invalid Opcode" << endl;
						failedLine = true;
						break;
					}
				}else{
					cerr << "ERROR : Invalid Opcode" << endl;
					failedLine = true;
					break;
				}
				first = false;
			}else{
				onlyLabel = false;
				if(token.getKind() == "WORD"){
					if(!labeledLine){
						cerr << "ERROR : Need integer after directive .word" << endl;
						failedLine = true;
						break;
					}else{
						wordOccurred = true;
					}
				}else if(token.getKind() == "HEXINT" || token.getKind() == "INT"){
					if(prev.getKind() == "HEXINT" || prev.getKind() == "INT" || prev.getKind() == "ID" || prev.getKind() == "REG"){
						cerr << "ERROR : Expected end of line, but there's more stuff" << endl;
						failedLine = true;
						break;
					}else if(storeword || loadword){
						if(token.getKind() == "HEXINT"){
							if(token.toLong() <= 0xffff && token.toLong() >= 0){
								bneNum = token.toLong();
							}else{
								cerr << "ERROR : Expected a 16 bit number but received more" << endl;
								failedLine = true;
								break;
							}
						}else if(token.getKind() == "INT"){
							if(token.toLong() <= 32767 && token.toLong() >= -32768){
								bneNum = token.toLong();
							}else{
								cerr << "ERROR : Expected a 16 bit number but received more" << endl;
								failedLine = true;
								break;
							}
						}
						if(numRegs == 1 && !numberOcc){
							if(prev.getKind() != "COMMA"){
								cerr << "ERROR : Expected a COMMA before this integer" << endl;
								failedLine = true;
								break;
							}
							loadwordInt = token.toLong();
							numberOcc = true;
						}else{
							cerr << "ERROR : Expected a 16 bit number but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(bne || beq){
						if(numRegs == 2){
							if(token.getKind() == "HEXINT"){
								if(token.toLong() <= 0xffff && token.toLong() >= 0){
									numRegs += 1;
									bneNum = token.toLong();
								}else{
									cerr << "ERROR : Expected a 16 bit number but received more" << endl;
									failedLine = true;
									break;
								}
							}else if(token.getKind() == "INT"){
								if(token.toLong() <= 32767 && token.toLong() >= -32768){
									numRegs += 1;
									bneNum = token.toLong();
								}else{
									cerr << "ERROR : Expected a 16 bit number but received more" << endl;
									failedLine = true;
									break;
								}
							}
						}else{
							cerr << "ERROR : Expected 3 fields" << endl;
							failedLine = true;
							break;
						}
					}else{
						added = true;
						printThis = token.toLong();
					}
				}else if(token.getKind() == "LABEL"){
					if(prev.getKind() == "WORD" || prev.getKind() == "HEXINT" || prev.getKind() == "INT" || prev.getKind() == "REG" || prev.getKind() == "ID"){
						cerr << "ERROR : Cannot define label after another opcode" << endl;
						failedLine = true;
						break;
					}else{
						labeledLine = true;
						onlyLabel = true;
						if(validLabel(token.getLexeme())){
							string s = token.getLexeme();
							s.pop_back();
							if(st.count(s) != 0){//(st.find(token.getLexeme().pop_back()) != st.end()){
								cerr << "ERROR : Duplicate definition of a label" << endl;
								failedLine = true;
								break;
							}else{
								st.insert(pair<string,int>(s,lineCounter * 4));//[token.getLexeme().pop_back()] = lineCounter * 4;
							}
						}else{
							cerr << "ERROR : Not a valid label" << endl;
							failedLine = true;
							break;
						}
					}
				}else if(token.getKind() == "ID"){
					if(prev.getKind() =="WORD"){
						added = true;
					}else if(bne || beq || labeledLine){
						if(token.getLexeme() == "bne"){
							bne = true;
						}else if(token.getLexeme() == "beq"){
							beq = true;
						}else{
							cerr << "ERROR : Expected end of line but got more stuff" << endl;
							failedLine = true;
							break;
						}
					}else{
						cerr << "ERROR : Expected end of line but got more stuff" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "COMMA"){
					if(prev.getKind() != "REG"){
						cerr << "ERROR : Expected comma after register" << endl;
						failedLine = true;
						break;
					}else{
						if(mult || multu || divi || diviu){
							if(numRegs == 2){
								cerr << "ERROR : Expected end of line but got more" << endl;
								failedLine = true;
								break;
							}
						}
					}
				}else if(token.getKind() == "LPAREN"){
					if(prev.getKind() == "INT" || prev.getKind() == "HEXINT"){
						
					}else{
						cerr << "ERROR : Expected integer or hex number before LPAREN" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "RPAREN"){
					if(prev.getKind() != "REG"){
						cerr << "ERROR : Expected register before RPAREN" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "REG"){
					if(jrjalr){

					}else if(mult || multu || divi || diviu){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 2 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if((mflo || mfhi || lis) && !gotReg){
						reg1 = token.toInt();
						gotReg = true;
					}else if(addSub || slt){
						if(numRegs < 3){
							if(numRegs == 0){
								if(prev.getLexeme() == "sub"){
									subtract = true;
								}else if(prev.getLexeme() == "sltu"){
									unsignedSlt = true;
								}
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}else if(numRegs == 2){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg3 = token.toInt();
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(beq || bne){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 2 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(storeword || loadword){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "LPAREN"){
									cerr << "ERROR : Expected a left parentasis" << endl;
									failedLine = true;
									break;
								}else if(numberOcc){
									reg2 = token.toInt();
								}else{
									cerr << "ERROR : Expected a number before the second register" << endl;
									failedLine = true;
									break;
								}
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 2 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else{
						cerr << storeword << " " << loadword << endl;
						cerr << "ERROR : unknown error1" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getLexeme() == "jr" || token.getLexeme() == "jalr"){
					if(!labeledLine){
						cerr << "ERROR : Need integer after directive .word" << endl;
						failedLine = true;
						break;
					}else{
						jrjalr = true;
					}
				}else{
					cerr << "ERROR : unknown error2" << endl;
					failedLine = true;
					break;
				}
			}
			prev = token;
		}
		if(addSub){
			if (numRegs == 3){
				added = true;
				if(subtract){
					printThis = setUpAddSub(reg1,reg2,reg3,true);
				}else{
					printThis = setUpAddSub(reg1,reg2,reg3,false);
				}
			}else{
				cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
				failedLine = true;
				break;
			}
		}else if(slt){
			if (numRegs == 3){
				added = true;
				if(unsignedSlt){
					printThis = setUpslt(reg1,reg2,reg3,true);
				}else{
					printThis = setUpslt(reg1,reg2,reg3,false);
				}
			}else{
				cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
				failedLine = true;
				break;
			}
		}
		if(failedLine){
			cerr << "ERROR : one or more failed lines, aborting assembling" << endl;
			break;
		}else if(executedLoop && added){
			if(!onlyLabel){
				lineCounter = lineCounter + 1;
			}
		}else if(executedLoop){
			if(!onlyLabel){
				lineCounter = lineCounter + 1;
			}
		}else if(!failedLine && prev.getKind() == "WORD"){
			cerr << "ERROR : Expected a numerical value after .word" << endl;
			failedLine = true;
			break;
		}
		executedLoop = false;
		first = true;
  	}
	return pair<bool,map<string,int>>(failedLine,st);
}

void assemble(vector<vector<Token>> &tokenLines,vector<int> &printQueue, map<string,int>&st) {
	pair<bool,map<string,int>> stCheck = constructSymbolTable(tokenLines);
	if (stCheck.first){
		return;
	}
	st = stCheck.second;
	bool failedLine = false;
	int lineCounter = 0;
	for (auto &line : tokenLines) {
	// For now just print out the tokens, one line at a time.
	// You will probably want to replace this with your code.
		Token prev = Token("PLACEHOLDER","0x00000000");
		bool first = true;
		bool executedLoop = false;
		bool wordOccurred = false;
		bool jrjalr = false;
		bool added = false;
		bool labeledLine = false;
		bool addSub = false;
		bool subtract = false;
		bool onlyLabel = false;
		bool unsignedSlt = false;
		bool slt = false;
		int numRegs = 0;
		int reg1 = -1;
		int reg2 = -1;
		int reg3 = -1;
		int printThis = 0;
		//beq stuff
		bool beq = false;
		bool bne = false;
		int bneNum = 0;
		//lis mfhi mflo stuff
		bool lis = false;
		bool mfhi = false;
		bool mflo = false;
		bool gotReg = false;
		//mult div stuff
		bool mult = false;
		bool divi = false;
		bool multu = false;
		bool diviu = false;
		//sw
		bool storeword = false;
		bool loadword = false;
		int loadwordInt = 0;
		bool numberOcc = false;
		for (auto &token : line) {
			executedLoop = true;
			//cerr << "Token(" << token.getKind() << "," << token.getLexeme() << ") ";
			if(first){
				if(token.getKind() == "WORD"){
					wordOccurred = true;
				}else if(token.getKind() == "LABEL"){
					labeledLine = true;
					onlyLabel = true;
				}else if(token.getKind() == "ID"){
					if(token.getLexeme() == "jr" || token.getLexeme() == "jalr"){
						jrjalr = true;
					}else if(token.getLexeme() == "add" || token.getLexeme() == "sub"){
						addSub = true;
					}else if(token.getLexeme() == "slt" || token.getLexeme() == "sltu"){
						slt = true;
					}else if(token.getLexeme() == "bne" || token.getLexeme() == "beq"){
						if(token.getLexeme() == "bne"){
							bne = true;
							beq = false;
						}else{
							beq = true;
							bne = false;
						}
					}else if(token.getLexeme() == "lis" || token.getLexeme() == "mfhi" || token.getLexeme() == "mflo"){
						if(token.getLexeme() == "lis"){
							lis = true;
						}else if(token.getLexeme() == "mfhi"){
							mfhi = true;
						}else if(token.getLexeme() == "mflo"){
							mflo = true;
						}
					}else if(token.getLexeme() == "mult" || token.getLexeme() == "div" || token.getLexeme() == "divu" || token.getLexeme() == "multu"){
						if(token.getLexeme() == "divu"){
							diviu = true;
						}else if(token.getLexeme() == "div"){
							divi = true;
						}else if(token.getLexeme() == "mult"){
							mult = true;
						}else if(token.getLexeme() == "multu"){
							multu = true;
						}
					}else if(token.getLexeme() == "sw" || token.getLexeme() == "lw"){
						if(token.getLexeme() == "sw"){
							storeword = true;
						}else{
							loadword = true;
						}
					}else{
						cerr << "ERROR : Invalid Opcode" << endl;
						failedLine = true;
						break;
					}
				}else{
					cerr << "ERROR : Invalid Opcode" << endl;
					failedLine = true;
					break;
				}
				first = false;
			}else{
				onlyLabel = false;
				if(token.getKind() == "WORD"){
					if(!labeledLine){
						cerr << "ERROR : Need integer after directive .word" << endl;
						failedLine = true;
						break;
					}else{
						wordOccurred = true;
					}
				}else if(token.getKind() == "HEXINT" || token.getKind() == "INT"){
					if(prev.getKind() == "HEXINT" || prev.getKind() == "INT" || prev.getKind() == "ID" || prev.getKind() == "REG"){
						cerr << "ERROR : Expected end of line, but there's more stuff" << endl;
						failedLine = true;
						break;
					}else if(storeword || loadword){
						if(token.getKind() == "HEXINT"){
							if(token.toLong() <= 0xffff && token.toLong() >= 0){
								bneNum = token.toLong();
							}else{
								cerr << "ERROR : Expected a 16 bit number but received more" << endl;
								failedLine = true;
								break;
							}
						}else if(token.getKind() == "INT"){
							if(token.toLong() <= 32767 && token.toLong() >= -32768){
								bneNum = token.toLong();
							}else{
								cerr << "ERROR : Expected a 16 bit number but received more" << endl;
								failedLine = true;
								break;
							}
						}
						if(numRegs == 1 && !numberOcc){
							if(prev.getKind() != "COMMA"){
								cerr << "ERROR : Expected a COMMA before this integer" << endl;
								failedLine = true;
								break;
							}
							loadwordInt = token.toLong();
							numberOcc = true;
						}else{
							cerr << "ERROR : Expected a 16 bit number but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(bne || beq){
						if(numRegs == 2){
							if(token.getKind() == "HEXINT"){
								if(token.toLong() <= 0xffff && token.toLong() >= 0){
									numRegs += 1;
									bneNum = token.toLong();
								}else{
									cerr << "ERROR : Expected a 16 bit number but received more" << endl;
									failedLine = true;
									break;
								}
							}else if(token.getKind() == "INT"){
								if(token.toLong() <= 32767 && token.toLong() >= -32768){
									numRegs += 1;
									bneNum = token.toLong();
								}else{
									cerr << "ERROR : Expected a 16 bit number but received more" << endl;
									failedLine = true;
									break;
								}
							}
						}else{
							cerr << "ERROR : Expected 3 fields" << endl;
							failedLine = true;
							break;
						}
					}else{
						added = true;
						printThis = token.toLong();
					}
				}else if(token.getKind() == "LABEL"){
					if(prev.getKind() == "WORD" || prev.getKind() == "HEXINT" || prev.getKind() == "INT" || prev.getKind() == "REG" || prev.getKind() == "ID"){
						cerr << "ERROR : Cannot define label after .word" << endl;
						failedLine = true;
						break;
					}else{
						labeledLine = true;
						onlyLabel = true;
					}//2/09 2pm
				}else if(token.getKind() == "ID"){
					if(labeledLine){
						if(token.getLexeme() == "bne"){
							bne = true;
						}else if(token.getLexeme() == "beq"){
							beq = true;
						}
						labeledLine = false;
					}else if(prev.getKind() =="WORD" || token.getLexeme() != "jr" || token.getLexeme() != "jalr"){
						if(bne || beq){
							string s = token.getLexeme();
							if(st.count(s) == 0){//(st.find(token.getLexeme().pop_back()) != st.end()){
								cerr << "ERROR : Label does not exist in the symbol table" << endl;
								failedLine = true;
								break;
							}
							if(prev.getKind() != "COMMA"){
								cerr << "ERROR : Expected a comma" << endl;
								failedLine = true;
								break;
							}
							bneNum = st[token.getLexeme()];
							bneNum = (bneNum - (lineCounter * 4) - 4) / 4;
							if(bneNum > 32767 || bneNum < -32768){
								cerr << "ERROR : Integer out of range" << endl;
								failedLine = true;
								break;
							}
							numRegs += 1;
						}else{
							added = true;
							string s = token.getLexeme();
							if(st.count(s) == 0){//(st.find(token.getLexeme().pop_back()) != st.end()){
								cerr << "ERROR : Label does not exist in the symbol table" << endl;
								failedLine = true;
								break;
							}
							printThis = st[token.getLexeme()];
						}
					}else{
						cerr << "ERROR : Expected end of line but got more stuff" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "COMMA"){
					if(prev.getKind() != "REG"){
						cerr << "ERROR : Expected register before comma" << endl;
						failedLine = true;
						break;
					}else if(mflo || mfhi || lis){
						cerr << "ERROR : Expected a register but got a COMMA" << endl;
						failedLine = true;
						break;
					}else if(mult || multu || divi || diviu){
						if(numRegs == 2){
							cerr << "ERROR : Expected end of line but got more" << endl;
							failedLine = true;
							break;
						}
					}
				}else if(token.getKind() == "LPAREN"){
					if(prev.getKind() == "INT" || prev.getKind() == "HEXINT"){
						
					}else{
						cerr << "ERROR : Expected integer or hex number before LPAREN" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "RPAREN"){
					if(prev.getKind() != "REG"){
						cerr << "ERROR : Expected register before RPAREN" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getKind() == "REG"){
					if(jrjalr){
						int regNumber = token.toInt();
						if(regNumber > 31 && regNumber < 0){
							cerr << "ERROR : unknown error3" << endl;
							failedLine = true;
							break;
						}
						added = true;
						if(prev.getLexeme() == "jalr"){
							cerr << "EXEC jalr" << endl;
							printThis = setUpjr(regNumber,true);
						}else{
							cerr << "EXEC jr" << endl;
							printThis = setUpjr(regNumber,false);
						}
					}else if(mult || multu || divi || diviu){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 2 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if((mflo || mfhi || lis) && !gotReg){
						reg1 = token.toInt();
						gotReg = true;
					}else if(addSub || slt){
						if(numRegs < 3){
							if(numRegs == 0){
								if(prev.getLexeme() == "sub"){
									subtract = true;
								}else if(prev.getLexeme() == "sltu"){
									unsignedSlt = true;
								}
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}else if(numRegs == 2){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg3 = token.toInt();
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(beq || bne){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
								reg2 = token.toInt();
							}else if(numRegs == 2){
								if(prev.getKind() != "COMMA"){
									cerr << "ERROR : Expected a comma" << endl;
									failedLine = true;
									break;
								}
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else if(storeword || loadword){
						if(numRegs < 2){
							if(numRegs == 0){
								reg1 = token.toInt();
							}else if(numRegs == 1){
								if(prev.getKind() != "LPAREN"){
									cerr << "ERROR : Expected a left parentasis" << endl;
									failedLine = true;
									break;
								}else if(numberOcc){
									reg2 = token.toInt();
								}else{
									cerr << "ERROR : Expected a number before the second register" << endl;
									failedLine = true;
									break;
								}
							}
							numRegs += 1;
						}else{
							cerr << "ERROR : Expected 2 Registers after opcode but received more" << endl;
							failedLine = true;
							break;
						}
					}else{
						cerr << "ERROR : unknown error4" << endl;
						failedLine = true;
						break;
					}
				}else if(token.getLexeme() == "jr" || token.getLexeme() == "jalr"){
					if(!labeledLine){
						cerr << "ERROR : Need integer after opcode" << endl;
						failedLine = true;
						break;
					}else{
						jrjalr = true;
					}
				}else{
					cerr << "ERROR : unknown error5" << endl;
					failedLine = true;
					break;
				}
			}
			prev = token;
		}
		if(addSub){
			if (numRegs == 3){
				added = true;
				if(subtract){
					printThis = setUpAddSub(reg1,reg2,reg3,true);
				}else{
					printThis = setUpAddSub(reg1,reg2,reg3,false);
				}
			}else{
				cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
				failedLine = true;
				break;
			}
		}else if(slt){
			if (numRegs == 3){
				added = true;
				if(unsignedSlt){
					printThis = setUpslt(reg1,reg2,reg3,true);
				}else{
					printThis = setUpslt(reg1,reg2,reg3,false);
				}
			}else{
				cerr << "ERROR : Expected 3 Registers after opcode but received more" << endl;
				failedLine = true;
				break;
			}
		}else if(bne || beq){
			if (numRegs == 3){
				added = true;
				printThis = setUpBne(reg1, reg2, bneNum, bne);
			}else{
				cerr << "ERROR : Expected one more number" << endl;
				failedLine = true;
				break;
			}
		}else if(mfhi || mflo || lis){
			if(gotReg){
				added = true;
				if(mfhi){
					printThis = setUplis(reg1, 2);
				}else if(mflo){
					printThis = setUplis(reg1, 3);
				}else if(lis){
					printThis = setUplis(reg1, 1);
				}
			}else{
				cerr << "ERROR : Expected one more register" << endl;
				failedLine = true;
				break;
			}
		}else if(mult || multu || divi || diviu){
			if(numRegs == 2){
				added = true;
				if(diviu){
					printThis = setUpMult(reg1, reg2, 3);
				}else if(divi){
					printThis = setUpMult(reg1, reg2, 2);
				}else if(mult){
					printThis = setUpMult(reg1, reg2, 0);
				}else if(multu){
					printThis = setUpMult(reg1, reg2, 1);
				}
			}else{
				cerr << "ERROR : You have too little regs" << endl;
				failedLine = true;
				break;
			}
		}else if(storeword || loadword){
			if(numberOcc && numRegs == 2){
				added = true;
				if(storeword){
					printThis = setUpSW(reg2, reg1, loadwordInt, false);
				}else if(loadword){
					printThis = setUpSW(reg2, reg1, loadwordInt, true);
				}
			}else{
				cerr << "ERROR : You have too little regs or you are missing an int" << endl;
				failedLine = true;
				break;
			}
		}
		if(failedLine){
			cerr << "ERROR : one or more failed lines, aborting assembling" << endl;
			break;
		}else if(executedLoop && added){
			printQueue.push_back(printThis);
			if(!onlyLabel){
				lineCounter = lineCounter + 1;
			}
		}else if(executedLoop){
			if(!onlyLabel){
				lineCounter = lineCounter + 1;
			}
		}else if(!failedLine && prev.getKind() == "WORD"){
			cerr << "ERROR : Expected a numerical value after .word" << endl;
			failedLine = true;
			break;
		}
		executedLoop = false;
		first = true;
  	}

  	if(!failedLine){
  		for (auto &printThis : printQueue) {
  			//cerr << bitset<32>(printThis) << endl;
  			putword(printThis);
		}
  	}
}
// Convert the input into a sequence of tokens.
// This should not need to be modified, but you can if you want to.
int main() {
  	CS241::AsmDFA theDFA;
  	vector<vector<Token>> tokenLines;
  	vector<int> printQueue;
  	string line;
  	map<string,int> st;

  	try {
  		while (getline(std::cin, line)) {
	  		tokenLines.push_back(theDFA.scan(line));
		}
  	} catch (CS241::ScanningFailure &f) {
		cerr << f.getMessage() << endl;

		return 1;
  	}

  	try {
		assemble(tokenLines,printQueue,st);
  	} catch (AssemblingFailure &f) {
		cerr << f.getMessage() << endl;
		return 1;
  	}
  	return 0;
}





