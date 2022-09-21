using namespace std;

/******* definizioni delle strutture dati *******/
enum valueType {temperature, humidity};

struct airValue {
	valueType valType;
	short index;
};

struct weatherProfile {
	double* airTemperature;
	double* airHumidity;
	double* dewPoint;
	double** MADsTemp;
	double** MADsHumi;

	double* pastDaysT;
	double* pastDaysH;

	short cFactor;
	bool oWinter;

	double maxT;
	double minT;
	double maxH;
	double minH;

	double maxTM;
	double minTM;
	double maxHM;
	double minHM;

	double* seasonalComponents; // seasonal components for prediction 

};
/************************************************/


/******* definizioni delle variabili globali *******/

const short nMADs = 19;
const short mNorth = 9;
const short mSouth = 10;
const short nSamples = 144;
const short nOriginalSamples = 48;
const short seasonLenght = 144;
const double e = 0.01;

const char* s1fileName = "data/sensor_1/s1_data.csv";
const char* s2fileName = "data/sensor_2/s2_data.csv";
const char* s3fileName = "data/sensor_3/s3_data.csv";

const char* outputFile = "../params_def_in.h";

airValue t = {temperature, 4};
// 4: indice della colonna contente i valori di temperatura nei dataset

airValue h = {humidity, 5};
// 5: indice della colonna contente i valori di umidita' nei dataset

/****************************************************/


/******* Opzioni di elaborazione dei dati *******/ 

int correctionFactor = 5;			  
bool onlyWinter = true;			

/************************************************/

/******* Dichiarazioni delle funzioni *******/

// Importa i dati dal file "fileName" e li inserisce in una matrice 
// implementata tramite vector
vector<vector<string> > importData(const char* fileName); 

// Funzione ausiliaria per stampare i dati importati
void printData(vector<vector<string> >  v);

// Riceve un dataset in ingresso e ne produce uno contenente soltanto i 
// dati relativi a mesi invernali del dataset in ingresso
vector<vector<string> > onlyWinterData(vector<vector<string> > data);

// Riceve un dataset in ingresso e produce in uscita un vettore contenente
// i valori di temperatura o umidita' (a seconda del parametro val)
// necessari a coprire l'arco di una simulazione (48 valori) partendo dal 
// dato con indice "start" all'interno del dataset. Applica correctionFactor.
double* initAirValues(vector<vector<string> > data, int start, airValue val);

// Riceve in ingresso un vettore di valori dell'aria (generato da initAirValues()) relativo ad "n" gg
// e lo espande portando la dim a 144*n (numero di valori necessari per coprire n
// simulazioni) interpolando i valori mancanti.
double* expandAirValues(double* airValues, int n);

// Riceve un dataset in ingresso e ricava i valori di temperatura o umidita' 
// (a seconda di "val") delle 2 giornate precedenti alla giornata che inizia da "start"
double* getPastDaysValues(vector<vector<string> > data, int start, airValue val);

// Riceve in ingresso un vettore contenente valori dell'aria (di temperatura o 
// umidita' a seconda del valori di val) e produce nMADs vettori 
// i cui dati rappresentano le rilevazioni dei MAD.
double** initMADsTemp(double* airValues, double* dewPoint);

double** initMADsHumi(double* airValues, double* dewPoint);

// Funzione ausiliaria per trovare max e min nel vettore values
void findMaxAndMin(double* values, double &max, double &min);

// Per ogni coppia (temperatura,umidita') calolca dew point e 
// restituisce i risultati in un vettore
double* initDewPoint(double* airTemperature, double* airHumidity);

// Dato un vettore di temperature di un mad e il relativo vettore di dew point
// calcola il vettore delle umidita' relative
double* calculateRH(double* MADsTemperature, double* dewPoint);

// Inizializza il vettore seasonalComponents per la predizione delle temperature 
// nella politica on/off utilizzando tutti i dati all'interno del dataset utilizzato
// per la creazione del profilo meteorologico
double* initSeasonalComponents(vector<vector<string> > dataSet);

void writeWeatherProfile(weatherProfile wp);

// Genera un nuovo profilo meteo
weatherProfile generateWeatherProfile();

/********************************************/

