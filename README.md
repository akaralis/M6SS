## About the project
The _Minimal 6TiSCH Synchronization Simulator_ (_M6SS_) is an ad-hoc simulator for the initial synchronization process
of 6TiSCH network in the context of the [minimal 6TiSCH configuration](https://tools.ietf.org/html/rfc8180). 
This project was developed in the context of the paper:
```
A. Karalis, D. Zorbas and C. Douligeris, "Optimal Initial Synchronization Time in the Minimal 6TiSCH Configuration" in IEEE Access. 
```
The language of the source code is C++.
## Code Organization
The structure of the source code is the following:
1. The files `syncparameters.h` and `syncparameters.cpp` respectively contain the definition and the implementation of a class named _SyncParameters_ that represents the synchronization parameters of the initial synchronization process in the minimal 6TiSCH configuration.
2. The files `timeinterval.h` and `timeinterval.cpp` respectively contain the definition and the implementation of a support class named _TimeInterval_ that represents a time interval.
3. The files `simulator.h` and `simulator.cpp` contain the core of the simulator, which is represented by a class named _Simulator_. It is noted that the simulator implements Αlgorithm 2 of the paper.
4. The files `model.h` and `model.cpp` respectively contain the definition and the implementation of a class named _Model_ that represents the mathematical model presented in the paper.
5. The files `modelvalidation.h` and `modelvalidation.cpp` respectively contain the definition and the implementation of a support class developed to validate the results of the model through a comparison with the results of the simulator. 
   In addition to the comparison between the model and the simulator, it also checks the validity of the optimal scan period 
   defined in the paper. All the (random) comparisons made during an execution of the validation code are stored in a database named modelValidation.db.
   An example of this database, which was generated for the needs of the paper, is in the folder [`results`](https://github.com/akaralis/M6SS/tree/master/results). 
6. The file `main.cpp` is the main file of the code where the execution starts. By default, it contains an example use of the simulator and of the model. 
   For formal reasons, it also contains a function called `generateSimStatsFig8` that was used for generating the simulator
   statistics presented in the Figure 8 of the paper. The data that are produced by this function are stored in a csv file
   named `simStatsFig8.csv`. An example of this file, which was used for the needs of the paper, is in the folder [`results`](https://github.com/akaralis/M6SS/tree/master/results).

## Prerequisites to run the code
To run the code the following are required:
1. A C++ compiler that supports the C++17 version.
2. [cmake](https://cmake.org/download/).

## License
This project follows the GNU Affero General Public License.