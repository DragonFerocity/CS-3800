#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <cstdlib>
using namespace std;

struct page {
  unsigned int vid;
  int rid;
  bool active;
  unsigned long timestamp;
  bool use;

  page(int i) : vid(i), rid(-1), active(false), use(false) {}
  page(int i, int time, int ri) : vid(i), rid(ri), active(true), timestamp(time) {}

  void activate(int memID, int time) {
    rid = memID;
    timestamp = time;
    active = true;
  }
  void activateClock(int memID) {
    rid = memID;
    use = true;
    active = true;
  }
  void deactivate() {
    rid = -1;
    active = false;
    use = false;
  }
};

struct pageTable {
  vector<page*> entry;

  pageTable(int start, int end) {
    for (int i = start; i <= end; i++)
      entry.push_back(new page(i));
  }
  ~pageTable() {
    for (unsigned int i = 0; i < entry.size(); i++)
      delete entry[i];
  }
};

struct program {
  unsigned int id;
  unsigned int size;
  unsigned int offset;
  pageTable* table;

  program(int memId, int prgmSize, int start, int end) : id(memId), size(prgmSize) {
    offset = start;
    table = new pageTable(start, end);
    cout << "Program " << memId << " has " << start << " to " << end << " page id's : " << start << ":" << end << endl;
  }
  ~program() {
    delete table;
  }
};

struct mainMemory {
  int memorySize;
  int emptyPageSpots;
  vector<page*> memory;

  mainMemory(int size, int pageSize) : memorySize(size) {
    emptyPageSpots = size / pageSize;
    memory.resize(emptyPageSpots);
  }

  void defaultLoadMemory(const int numPages, const vector<program*> & programs) {
    for (unsigned int i = 0; i < programs.size(); i++) {
      for (int k = 0; k < numPages; k++) {
        memory[i*numPages + k] = programs[i]->table->entry[k];
        programs[i]->table->entry[k]->activate(i*numPages + k, 0);
        emptyPageSpots -= 1;
      }
      //cout << "\t" << numPages << " pages loaded into main memory for program " << i << endl;
    }
    //cout << emptyPageSpots << " empty page locations in memory." << endl;
  }
  bool find(unsigned int id) {
    bool found = false;
    for (unsigned int i = 0; i < memory.size(); i++) {
      if (memory[i]->vid == id) {
        found = true;
        break;
      }
    }
    return found;
  }
  int findOldest() {
    unsigned int time = memory[0]->timestamp;
    int k = 0;

    for (unsigned int i = 1; i < memory.size(); i++) {
      if (memory[i]->timestamp < time) {
        time = memory[i]->timestamp;
        k = i;
      }
    }
    return k;
  }
  int findEmpty() {
    int k = -1;
    for (unsigned int i = 0; i < memory.size(); i++) {
      if (memory[i] == NULL) {
        k = i;
        break;
      }
    }
    return k;
  }
};

class OS {
public:
  mainMemory* mainMem;
  unsigned int pageSize;
  vector<program*> programs;
  vector<int> programTraces;
  unsigned long programCounter;

  OS(int ps) : pageSize(ps) {
    mainMem = new mainMemory(512, ps);
    programCounter = 0;
  }
  ~OS() {
    for (unsigned int i = 0; i < programs.size(); i++) {
      delete programs[i];
    }
    delete mainMem;
  }

  void setup(const string programListFile, const string programTraceFile) {
    loadPrograms(programListFile);
    loadProgramTraces(programTraceFile);
    mainMem->defaultLoadMemory(mainMem->memorySize / programs.size() / pageSize, programs);
  }

  void performLRU(const string pageType) {
    int program = 0;
    unsigned int location = 0;
    unsigned int relLocation = 0;
    unsigned long pageFaults = 0;
    unsigned int offset = 0;
    for (unsigned int i = 0; i < programTraces.size(); i += 2) {
      program = programTraces[i];
      offset = programs[program]->offset;
      location = (programTraces[i + 1] - 1) / pageSize + offset;
      relLocation = (programTraces[i + 1] - 1) / pageSize;
      cout << (i / 2) + 1 << ". Program " << program << " -> " << location << ": ";
      if (programs[program]->table->entry[relLocation]->active) {
        programs[program]->table->entry[relLocation]->timestamp = programCounter;
        cout << "In Memory! Timestamp set to " << programCounter << endl; //Nothing to do here!
      }
      else {
        if (mainMem->emptyPageSpots > 0) {
          int empty = mainMem->findEmpty();
          programs[program]->table->entry[relLocation]->activate(empty, programCounter);
          mainMem->memory[empty] = programs[program]->table->entry[relLocation];
          mainMem->emptyPageSpots -= 1;
          cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
          cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            int empty = mainMem->findEmpty();
            programs[program]->table->entry[relLocation + 1]->activate(empty, programCounter);
            mainMem->memory[empty] = programs[program]->table->entry[relLocation + 1];
            mainMem->emptyPageSpots -= 1;
            cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
            cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          }
        }
        else {
          int oldest = mainMem->findOldest();
          mainMem->memory[oldest]->deactivate();
          programs[program]->table->entry[relLocation]->activate(oldest, programCounter);
          mainMem->memory[oldest] = programs[program]->table->entry[relLocation];
          cout << "Not in Memory! Used Location " << oldest << " set at time " << programCounter << "!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            int oldest = mainMem->findOldest();
            mainMem->memory[oldest]->deactivate();
            programs[program]->table->entry[relLocation + 1]->activate(oldest, programCounter);
            mainMem->memory[oldest] = programs[program]->table->entry[relLocation + 1];
            cout << "Not in Memory! Used Location " << oldest << " set at time " << programCounter << "!" << endl;
          }
        }
        pageFaults += 1;
      }
      programCounter += 1;
    }
    cout << endl << pageFaults << " page faults occured!" << endl;
  }

  void performFIFO(const string pageType) {
    int program = 0;
    unsigned int location = 0;
    unsigned int relLocation = 0;
    unsigned long pageFaults = 0;
    unsigned int offset = 0;
    for (unsigned int i = 0; i < programTraces.size(); i += 2) {
      program = programTraces[i];
      offset = programs[program]->offset;
      location = (programTraces[i + 1] - 1) / pageSize + offset;
      relLocation = (programTraces[i + 1] - 1) / pageSize;
      cout << (i / 2) + 1 << ". Program " << program << " -> " << location << ": ";
      if (programs[program]->table->entry[relLocation]->active) {
        //programs[program]->table->entry[relLocation]->timestamp = programCounter;
        cout << "In Memory!" << endl; //Nothing to do here!
      }
      else {
        if (mainMem->emptyPageSpots > 0) {
          int empty = mainMem->findEmpty();
          programs[program]->table->entry[relLocation]->activate(empty, programCounter);
          mainMem->memory[empty] = programs[program]->table->entry[relLocation];
          mainMem->emptyPageSpots -= 1;
          cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
          cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            int empty = mainMem->findEmpty();
            programs[program]->table->entry[relLocation + 1]->activate(empty, programCounter);
            mainMem->memory[empty] = programs[program]->table->entry[relLocation + 1];
            mainMem->emptyPageSpots -= 1;
            cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
            cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          }
        }
        else {
          int oldest = mainMem->findOldest();
          mainMem->memory[oldest]->deactivate();
          programs[program]->table->entry[relLocation]->activate(oldest, programCounter);
          mainMem->memory[oldest] = programs[program]->table->entry[relLocation];
          cout << "Not in Memory! Used Location " << oldest << " set at time " << programCounter << "!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            int oldest = mainMem->findOldest();
            mainMem->memory[oldest]->deactivate();
            programs[program]->table->entry[relLocation + 1]->activate(oldest, programCounter);
            mainMem->memory[oldest] = programs[program]->table->entry[relLocation + 1];
            cout << "Not in Memory! Used Location " << oldest << " set at time " << programCounter << "!" << endl;
          }
        }
        pageFaults += 1;
      }
      programCounter += 1;
    }
    cout << endl << pageFaults << " page faults occured!" << endl;
  }

  void performCLOCK(const string pageType) {
    int program = 0;
    unsigned int clock = 0;
    unsigned int location = 0;
    unsigned int relLocation = 0;
    unsigned long pageFaults = 0;
    unsigned int offset = 0;
    for (unsigned int i = 0; i < programTraces.size(); i += 2) {
      program = programTraces[i];
      offset = programs[program]->offset;
      location = (programTraces[i + 1] - 1) / pageSize + offset;
      relLocation = (programTraces[i + 1] - 1) / pageSize;
      cout << (i / 2) + 1 << ". Program " << program << " -> " << location << ": ";
      if (programs[program]->table->entry[relLocation]->active) {
        programs[program]->table->entry[relLocation]->use = true;
        cout << "In Memory!" << endl; //Nothing to do here!
      }
      else {
        if (mainMem->emptyPageSpots > 0) {
          int empty = mainMem->findEmpty();
          programs[program]->table->entry[relLocation]->activateClock(empty);
          mainMem->memory[empty] = programs[program]->table->entry[relLocation];
          mainMem->emptyPageSpots -= 1;
          cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
          cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            int empty = mainMem->findEmpty();
            programs[program]->table->entry[relLocation + 1]->activateClock(empty);
            mainMem->memory[empty] = programs[program]->table->entry[relLocation + 1];
            mainMem->emptyPageSpots -= 1;
            cout << "Not in Memory! Empty Location " << empty << " set at time " << programCounter << "!" << endl;
            cout << "\t" << mainMem->emptyPageSpots << " empty pages left!" << endl;
          }
        }
        else {
          while (mainMem->memory[clock]->use) {
            mainMem->memory[clock]->use = false;
            clock++;
            if (clock >= mainMem->memory.size())
              clock = 0;
          }
          mainMem->memory[clock]->deactivate();
          programs[program]->table->entry[relLocation]->activateClock(clock);
          mainMem->memory[clock] = programs[program]->table->entry[relLocation];
          cout << "Not in Memory! Used Location " << clock << " set at time " << programCounter << "!" << endl;
          if (pageType == "p" && relLocation < programs[program]->table->entry.size() - 1) {
            while (mainMem->memory[clock]->use) {
              mainMem->memory[clock]->use = false;
              clock++;
              if (clock >= mainMem->memory.size())
                clock = 0;
            }
            mainMem->memory[clock]->deactivate();
            programs[program]->table->entry[relLocation+1]->activateClock(clock);
            mainMem->memory[clock] = programs[program]->table->entry[relLocation+1];
            cout << "Not in Memory! Used Location " << clock << " set at time " << programCounter << "!" << endl;
          }
        }
        pageFaults += 1;
      }
      programCounter += 1;
    }
    cout << endl << pageFaults << " page faults occured!" << endl;
  }

private:
  void loadPrograms(const string programListFile) {
    ifstream in;
    int inVal;
    int size;
    int id;
    int start = 0;
    int end = 0;

    in.open(programListFile.c_str());
    while (in >> inVal) {
      id = inVal;
      in >> size;

      end = start + size / pageSize - (pageSize == 1 ? 1 : 0);
      programs.push_back(new program(id, size / pageSize, start, end));
      start = end + 1;
    }
    cout << programs.size() << " programs loaded." << endl;
  }
  void loadProgramTraces(const string programTraceFile) {
    ifstream in;
    int inVal;

    in.open(programTraceFile.c_str());

    while (in >> inVal)
      programTraces.push_back(inVal);

    cout << programTraces.size() / 2 << " traces loaded." << endl;
  }
};


void showInputHelp() {
  cout << "Error: Invalid arguments supplied!" << endl;
  cout << "memorysimulator programlist programtrace n method paging-type" << endl;
  cout << "\tprogramlist:" << endl << "\t\tname of programlist file" << endl;
  cout << "\tprogramtrace:" << endl << "\t\tname of programtrace file" << endl;
  cout << "\tn:" << endl << "\t\tsize of pages. Acceptable values are 1, 2, 4, 8, and 16" << endl;
  cout << "\tmethod:" << endl << "\t\tpage replacement method. Acceptable values are lru, fifo, and clock" << endl;
  cout << "\tpaging-type:" << endl << "\t\tdemand paging (d), or prepaging (p)." << endl;
}


int main(int numParams, char* args[]) {
  if (numParams == 6) {
    const string prglsFile = string(args[1]) + ".txt";
    const string cmdlsFile = string(args[2]) + ".txt";
    const int pgSize = atoi(args[3]);
    const string method = args[4];
    const string pagingType = args[5];

    if ((pgSize != 1 && pgSize != 2 && pgSize != 4 && pgSize != 8 && pgSize != 16) ||
      (method != "lru" && method != "fifo" && method != "clock") ||
      (pagingType != "d" && pagingType != "p"))
      showInputHelp();
    else {
      OS os(pgSize);
      os.setup(prglsFile, cmdlsFile);
      if (method == "lru")
        os.performLRU(pagingType);
      if (method == "fifo")
        os.performFIFO(pagingType);
      if (method == "clock")
        os.performCLOCK(pagingType);
    }
  }

  return 0;
}
