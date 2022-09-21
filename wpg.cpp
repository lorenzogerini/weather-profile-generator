#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <time.h>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <random>
#include "wpg.h"
using namespace std;

/****** Implementazione delle funzioni ******/

vector<vector<string> > importData(const char* fileName) {
	// Dato un file x.csv ne legge il contenuto e lo alloca in una matrice 
	fstream f;
	f.open(fileName, ios::in);

	vector<vector<string> > data;
	vector<string> row;
	string line, word;

	while (getline(f, line)) {

		row.clear();
		// read an entire row and 
        // store it in a string variable 'line' 
        // used for breaking words 
        stringstream s(line);

        // read every column data of a row and 
        // store it in a string variable, 'word' 
        while (getline(s, word, ',')) { 
            // add all the column data 
            // of a row to a vector 
            row.push_back(word); 
        } 
        data.push_back(row);
	}
	return data;
}

void printData(vector<vector<string> >  v) {
  cout << "DATA FROM SENSOR N. " << v[1][1] << endl << endl;
  for (int i = 0; i<v.size(); i++) {
    for (int j = 0; j<v[i].size(); j++) {
      cout << v[i][j] << " ";
    }
    cout << endl;
  }
  cout << endl << endl;
}

vector<vector<string> > onlyWinterData(vector<vector<string> > data) {
	//crea dataset di soli dati invernali a partire da uno completo
	vector<vector<string> > owData;
	for (int i = 0; i<data.size(); i++){
		string strDate = data[i][3]; 	// col n. 3 == date
		char str[strDate.size()+1];		// conversione da string a char*
		strcpy(str, strDate.c_str());	//
		struct tm date;
		strptime(str, "%Y-%m-%d", &date);
		if (date.tm_mon == 0 || date.tm_mon == 1 || date.tm_mon == 11)
			owData.push_back(data[i]);
	}
	return owData;
}

double* initAirValues(vector<vector<string> > data, int start, airValue val) {
	double* airValues = new double[nOriginalSamples];
	for (int i = 0; i<nOriginalSamples; i++){
		if (val.valType == temperature)
			airValues[i] = stod(data[start+i][val.index]) - correctionFactor;
		else if (val.valType == humidity)
			airValues[i] = stod(data[start+i][val.index]) / 100;
	}

	return airValues;
}

// ricava i dati relativi alle "n" giornate precedenti a partire da start
double* getPastDaysValues(vector<vector<string> > data, int start, airValue val){
	double* pastDaysVals = new double[nOriginalSamples*2];
	start = start - nOriginalSamples*2;
	for (int i = 0; i < nOriginalSamples*2; i++) {
		if (val.valType == temperature)
			pastDaysVals[i] = stod(data[start+i][val.index]) - correctionFactor;
		else if (val.valType == humidity)
			pastDaysVals[i] = stod(data[start+i][val.index]) / 100;
	}
	return pastDaysVals;
}

double* expandAirValues(double* values, int n){
	double* expandedValues = new double[nSamples*n];
	double precValue, nextValue;
	double avarage = 0;
	double newVal1 = 0;
	double newVal2 = 0;
	int expInd = 0;

	for (int i = 0; i < nOriginalSamples*n; i++) {
		precValue = values[i];
		nextValue = (i<nOriginalSamples-1)?values[i+1]:precValue;
		newVal1 = precValue + ((nextValue-precValue)/3)*(2-1);
		newVal2 = precValue + ((nextValue-precValue)/3)*(3-1);
		expandedValues[i+expInd] = precValue;
		expandedValues[i+1+expInd] = newVal1;
		expandedValues[i+2+expInd] = newVal2;
		expandedValues[i+3+expInd] = nextValue;
		expInd = expInd + 2;
	}
//	for (int i = 0; i < nSamples; i++) 
//		cout << i << " " << expandedValues[i] << " " << endl; 
//	cout << endl << endl;
	return expandedValues;
}

double* initDewPoint(double* airTemperature, double* airHumidity) {
	double* dewPoint = new double[nSamples];
	double a = 17.27;
	double b = 237.7;
	double alpha;
	for (int i = 0; i < nSamples; i++) {
		alpha = a*airTemperature[i]/(b+airTemperature[i]) + log(airHumidity[i]);
		dewPoint[i] = b*alpha/(a-alpha);
	}
	return dewPoint;
}

double* calculateRH(double* MADsTemperature, double* dewPoint) { 
	double* relativeHumidity = new double[nSamples];
	double a = 17.27;
	double b = 237.7;
	for (int i = 0; i < nSamples; i++) {
		double firstTerm = a*dewPoint[i]/(b+dewPoint[i]);
		double secondTerm = a*MADsTemperature[i]/(b+MADsTemperature[i]);
		double aus = firstTerm - secondTerm;
		relativeHumidity[i] = exp(aus);
		if (relativeHumidity[i] > 1)
			relativeHumidity[i] = 1;
		if (relativeHumidity[i] < 0.1)
			relativeHumidity[i] = 0.1;
	}
	return relativeHumidity;
}

double** initMADsTemp(double* airValues, double* dewPoint) {
	double** MADsValues = new double*[nMADs];
	for (int i = 0; i<nMADs; i++) {
		MADsValues[i] = new double[nSamples];
		double countUp = 0;
		double countDown = 0;
		double probUp = 0;
		for (int j = 0; j<nSamples; j++) {

			// cerco minPosVar-maxPosVar e minNegVar-maxNegVar delle temperature contenute in [airValues[j]-e; airValues[j]+e];
			double maxPosVariation = 0;
			double minPosVariation = 0;
			double maxNegVariation = 0;
			double minNegVariation = 0;

			if (j == 0) {							// fascia oraria 00:00-->07:00
				countUp = 0;
				countDown = 0;	
				for (int k = 0; k < 41; k++) {
					if (airValues[k+1] > airValues[k])
						countUp++;
					else 
						countDown++;
				}
				MADsValues[i][j] = airValues[j];
				continue;					
			}

			if (j == 42) {							// fascia oraria 07:00-->13:30
				countUp = 0;
				countDown = 0;	
				for (int k = 42; k < 80; k++) {
					if (airValues[k+1] > airValues[k])
						countUp++;
					else 
						countDown++;
				}					
			}

			if (j == 81) {							// fascia oraria 13:30-->20:00
				countUp = 0;
				countDown = 0;	
				for (int k = 81; k < 119; k++) {
					if (airValues[k+1] > airValues[k])
						countUp++;
					else 
						countDown++;
				}					
			}
		
			if (j == 120) {							// fascia oraria 20:00-->24:00
				countUp = 0;
				countDown = 0;	
				for (int k = 120; k < 143; k++) {
					if (airValues[k+1] > airValues[k])
						countUp++;
					else 
						countDown++;
				}					
			}

			for (int k = 0; k<nSamples; k++) {
				if (airValues[k] <= airValues[j]+e && airValues[k] >= airValues[j]-e){
					if (k != nSamples-1) {

						if (airValues[k+1] > airValues[k]) {
							if (maxPosVariation == 0)
								maxPosVariation = minPosVariation = airValues[k+1] - airValues[k];
							else if (airValues[k+1] - airValues[k] > maxPosVariation)
								maxPosVariation = airValues[k+1] - airValues[k];
							else if (airValues[k+1] - airValues[k] < minPosVariation)
								minPosVariation = airValues[k+1] - airValues[k];
						} else if (airValues[k+1] < airValues[k]) {
							if (maxNegVariation == 0) 
								maxNegVariation = minNegVariation = airValues[k] - airValues[k+1];
							else if (airValues[k] - airValues[k+1] > maxNegVariation)
								maxNegVariation = airValues[k] - airValues[k+1];
							else if (airValues[k] - airValues[k+1] < minNegVariation)
								minNegVariation = airValues[k] - airValues[k+1];
						}
					}
				}
			}
			double upProb = countUp/(countUp+countDown);
			if (i == 0)
				srand(j*time(NULL));
			else 
				srand(i*j*time(NULL));
			double choice = (double)rand() / RAND_MAX;
			double randomVariation;

			if (choice <= upProb) {
				// incremento: genero variazione positiva random tra minPosV e maxPosV
				uniform_real_distribution<double> unif(minPosVariation, maxPosVariation);
				default_random_engine re;
				randomVariation = unif(re);
				MADsValues[i][j] = MADsValues[i][j-1]+randomVariation;
			} else {
				// decremento: genero variazione negativa random tra minNegV e maxNegV
				uniform_real_distribution<double> unif(minNegVariation, maxNegVariation);
				default_random_engine re;
				randomVariation = unif(re);
				MADsValues[i][j] = MADsValues[i][j-1]-randomVariation;
			}
			if (i < mNorth) {
					uniform_real_distribution<double> unif(0, 0.5);
					default_random_engine re;
					randomVariation = unif(re);
					MADsValues[i][j] = MADsValues[i][j] - randomVariation;
			} 
			// controllo di validità
			if (MADsValues[i][j] > airValues[j]+4)
				MADsValues[i][j] = airValues[j]+4;
			if (MADsValues[i][j] < airValues[j]-4)
				MADsValues[i][j] = airValues[j]-4;
			if (MADsValues[i][j] < dewPoint[j])
				MADsValues[i][j] = dewPoint[j];

		}
	}
	return MADsValues;
}

double** initMADsHumi(double** MADsTemp, double* dewPoint) {
	double** MADsValues = new double *[nMADs];
	for (int i = 0; i < nMADs; i++) {
		MADsValues[i] = new double[nSamples];
		double* arrayT = new double[nSamples];
		for (int j = 0; j<nSamples; j++)
			arrayT[j] = MADsTemp[i][j];
		MADsValues[i] = calculateRH(arrayT, dewPoint);
	}
	return MADsValues;
} 

void findMaxAndMin(double* values, double &max, double &min) {
	max = values[0];
	min = values[0];
	for (int i = 0; i<nSamples; i++) {
		if (values[i]>max)
			max = values[i];
		if (values[i]<min)
			min = values[i];
	}
}

void findMaxAndMinMAD(double** mad, double &max, double &min) {
	max = mad[0][0];
	min = mad[0][0];
	for (int i = 0; i < nMADs; i++) {
		double tempMax = 0;
		double tempMin = 0;
		findMaxAndMin(&mad[i][0], tempMax, tempMin);
		if (tempMax > max)
			max = tempMax;
		if (tempMin < min)
			min = tempMin;
	}
}

// seasonal components elaborati da wpg in quanto i dati del profilo meteorologico vengono elaborati
// e scritti in params_def_in da qui. Successivamente non sarebbe possibile recuperare tali dati da 
// SAN_functions.h partendo da quelli originali.
double* initSeasonalComponents(vector<vector<string> > dataSet) {
  double *data = new double [dataSet.size()-1];
  for (int i = 1; i < dataSet.size(); i++)
	data[i-1] = stod(dataSet[i][4]);

  short nCycles = (dataSet.size()-1) / seasonLenght;

  double* avg = new double[nCycles];
  double* seasonalComponents = new double[seasonLenght];
  double sum = 0;

  for (int j = 0; j < nCycles; j++) {
    sum = 0;
    for (int i = 0; i < seasonLenght; i++)
      sum = sum + data[seasonLenght*j+i];
    sum = sum / seasonLenght;
    avg[j] = sum;
  }

  for (int i = 0; i < seasonLenght; i++) {
    sum = 0;
    for (int j = 0; j < nCycles; j++) 
      sum = sum + data[seasonLenght*j+i] / avg[j];
    seasonalComponents[i] = sum / nCycles;
  }
  return seasonalComponents;
}

weatherProfile generateWeatherProfile() {
	int randDataIndex;						// indice del dataset scelto randomicamente tra s1,s2 e s3
	int randSample;							// indice del sample scelto randomicamente
	vector<vector<string> > randData;		// dataset scelto randomicamente
	int offset;

	vector<vector<string> > s1Data = importData(s1fileName);
	//vector<vector<string> > s2Data = importData(s2fileName);
	vector<vector<string> > s3Data = importData(s3fileName);

	weatherProfile wp;

	srand(time(NULL));

	vector<vector<string> > dataList[2] = {s1Data, s3Data};	
	randDataIndex = rand() % 2;
	// dataset 2 rimosso 

	if (onlyWinter) 
		// se si desiderano solo temperature invernali i dati del sensor n. 2 vengono 
		// esclusi poiche' non ne contengono
		randData = onlyWinterData(dataList[randDataIndex]);		
		//riduce i dati del dataset scelto ai soli dati invernali dello stesso
	else 
		randData = dataList[randDataIndex];

	srand(time(NULL));

	randSample = rand() % (randData.size()-nOriginalSamples) + nOriginalSamples*2+1;
	offset = randSample % nSamples;
	randSample = randSample - offset; 	
	// faccio in modo che randSample corrisponda all'inizio della giornata

	wp.airTemperature = expandAirValues(initAirValues(randData, randSample, t), 1);			
	//vettore con valori della temperatura dell'aria
	
	wp.airHumidity = expandAirValues(initAirValues(randData, randSample, h), 1);				
	//vettore con valori dell'umidita' dell'aria

	wp.pastDaysT = expandAirValues(getPastDaysValues(randData, randSample, t), 2);
	// vettore con valori temp. dei 2 gg precedenti

	wp.pastDaysH = expandAirValues(getPastDaysValues(randData, randSample, h), 2);
	// vettore con valori umidita' dei 2 gg precedenti

	wp.dewPoint = initDewPoint(wp.airTemperature, wp.airHumidity);
	//vettore contente i dewpoint

	wp.MADsTemp = initMADsTemp(wp.airTemperature, wp.dewPoint);					
	//vettori con valori delle temperature dei mad
	
	wp.MADsHumi = initMADsHumi(wp.MADsTemp, wp.dewPoint);						
	//vettori con valori delle umidita' dei mad

	wp.cFactor = correctionFactor;
	wp.oWinter = onlyWinter;

	findMaxAndMin(wp.airTemperature, wp.maxT, wp.minT);
	findMaxAndMin(wp.airHumidity, wp.maxH, wp.minH);
	findMaxAndMinMAD(wp.MADsTemp, wp.maxTM, wp.minTM);
	findMaxAndMinMAD(wp.MADsHumi, wp.maxHM, wp.minHM);

	wp.seasonalComponents = initSeasonalComponents(randData);

	return wp;
}

void writeWeatherProfile(weatherProfile wp) {
	fstream file;
	file.open(outputFile, ios::app);

	string mode = (onlyWinter)?"on":"off";

	file << "#ifdef PROFILO_X" << endl;
	file << "// Range Temperature aria: [" << wp.minT << ", " << wp.maxT << "]" << endl;
	file << "// Range Umidita' aria: [" << wp.minH << ", " << wp.maxH << "]" << endl;
	file << endl;
	file << "// Range Temperature MADs: [" << wp.minTM << ", " << wp.maxTM << "]" << endl;
	file << "// Range Umidita' MADs: [" << wp.minHM << ", " << wp.maxHM << "]" << endl;
	file << endl;
	file << "// Only Winter Temperature: " << mode << endl;
	file << "// Fattore di Correzione: " << correctionFactor << "°C" << endl;
	file << endl;
	file << "// Valori dell'aria dei 2 giorni precedenti: " << endl;
	file << "const double pastDaysT[] = {";
	for (int i = 0; i < nSamples*2; i++) {
		file << wp.pastDaysT[i];
		if (i == nSamples*2-1)
			file << "};" << endl;
		else 
			file << ", ";
	}
	file << "const double pastDaysH[] = {";
	for (int i = 0; i < nSamples*2; i++) {
		file << wp.pastDaysH[i];
		if (i == nSamples*2-1)
			file << "};" << endl;
		else 
			file << ", ";
	}
	file << endl;
	file << "// Seasonal Components predizione delle temperature: " << endl;
	file << "const double wp_seasonalComponents[] = {";
	for (int i = 0; i < seasonLenght; i++) {
		file << wp.seasonalComponents[i];
		if (i == seasonLenght-1)
			file << "};" << endl;
		else 
			file << ", ";
	}
	file << endl;
	file << "const Array2D<double, nMADs+1, nSamples> temperature({" << endl;
	for (int i = 0; i < nMADs+1; i++) {
		for (int j = 0; j < nSamples; j++){
			if (j == 0)
				file << "{";
			if (i == nMADs)
				file << fixed << setprecision(6) << wp.airTemperature[j];
			else 
				file << fixed << setprecision(6) << wp.MADsTemp[i][j];
			if (j == nSamples-1)
					file << "}," << endl;
			else 
				file << ", ";
		}
	}
	file << "});" << endl;
	file << endl;
	file << "const Array2D<double, nMADs+1, nSamples> humidity({" << endl;
	for (int i = 0; i < nMADs+1; i++) {
		for (int j = 0; j < nSamples; j++){
			if (j == 0)
				file << "{";
			if (i == nMADs)
				file << fixed << setprecision(6) << wp.airHumidity[j];
			else {
				if (j == 0)
					file << fixed << setprecision(6) << wp.airHumidity[j];
				else
					file << fixed << setprecision(6) << wp.MADsHumi[i][j];
			}
			if (j == nSamples-1)
					file << "}," << endl;
			else 
				file << ", ";
		}
	}
	file << "});" << endl; 
	file << endl;
	file << "#endif" << endl;
	file << endl;
}

int main(int argc, char** argv) {

	/* elaborazione argomenti */
	if (argv[1]) {
		stringstream arg(argv[1]);
		int cf = 0;
		arg >> cf;
		if (cf < 0 || cf > 20) {
			cout << "Volore di correctionFactor non valido. Inserire un valore compreso tra 0 e 20" << endl;
			return 0;
		} else 
		correctionFactor = cf;	
	}
	if (argv[2] && argc > 2) {
		stringstream arg(argv[2]);
		int ow = 3;
		arg >> ow;
		if (ow == 0 || ow == 1) {
			onlyWinter = ow;
		} else {
			cout << "Valore di onlyWinter non valido. Inserire 0 o 1" << endl;
			return 0;
		}
	}
	/*************************/

	weatherProfile wp = generateWeatherProfile();
	writeWeatherProfile(wp);

	return 0;
}



