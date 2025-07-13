#include <iostream>
#include <fstream>
#include <vector>

#include <sstream>

#include <queue>

using namespace std;


char flipBit(char byte, int bitPos) {
    return byte ^ (1 << (bitPos - 1)); 
}


void processFile(const string& inputFile, const string& outputFile, int bitPos) {
    ifstream inFile(inputFile, ios::binary);  // Abrir en modo binario
    ofstream outFile(outputFile, ios::binary);

    if (!inFile || !outFile) {
        cerr << "Error al abrir los archivos." << endl;
        return;
    }

    char byte;
    while (inFile.read(&byte, sizeof(char))) {
        outFile.put(flipBit(byte, bitPos));  // Aplicar flipBit y escribir
    }

    inFile.close();
    outFile.close();
}


queue<string> leer_lista(const string& nombreArchivo) {
    queue<string> lineas;
    ifstream archivo(nombreArchivo);
    string linea;

    if (!archivo) {
        cerr << "No se pudo abrir el archivo: " << nombreArchivo << endl;
        return lineas;
    }

    while (getline(archivo, linea)) {
        lineas.push(linea);
        cout<< linea<<endl;
    }

    archivo.close();
    return lineas;
}





string delete_ext(const string& file, const string& extension) {
    if (file.size() >= extension.size() &&
        file.compare(file.size() - extension.size(), extension.size(), extension) == 0) {
        return file.substr(0, file.size() - extension.size());
    }
    return file;
}




int main() {



    int op = 0;
    string target;
    int bitdef;


    


    while (true) {

        
        cout << "OPCIONES" << endl;

        cout << "0. Salir" << endl;
        cout << "1. Encriptar un archivo" << endl;
        cout << "2. Desencriptar un archivo" << endl;
        cout << "3. Encriptar lista de archivos" << endl;
        cout << "4. desencriptar lista de archivos " << endl;
        cout << "Seleccione una opción: ";
        op = 0;
        cin >> op;

        switch (op) {

        case 0:
            return 0;
        case 1:
            cout << "Ingrese el nombre del archivo" << endl;
            cin >> target;
            processFile(target, target + ".PedroDorta", 5);
            break;
        case 2:
            cout << "Ingrese el nombre del archivo a desencriptar" << endl;
            cin >> target;
            processFile(target, delete_ext(target, ".PedroDorta"), 5);
            break;
        case 3: {
            cout << "Ingrese el nombre/ruta del archivo con la lista de objetivos" << endl;
            cin >> target;
            queue<string> list = leer_lista(target);

            cout << "se leyeron:" << list.size() << endl;
            
            system("pause");

            
                while (!list.empty()) {
                    cout << "[Corrompiendo :D]:" << list.front() << endl;
                    processFile(list.front(),  list.front() + ".PedroDorta" , 5);
                    list.pop();
                }



            break;
        }
        case 4: {
            cout << "Ingrese el nombre/ruta del archivo con la lista de objetivos corrompidos" << endl;
            cin >> target;
            queue<string> list = leer_lista(target);

            cout << "se leyeron:" << list.size() << endl;

            system("pause");

            cout <<"¿todos los archivos de la lista tienen la extencion .PedroDorta?" << endl;
            cout << "en caso de ser la misma lista de objetivos, presione (2)" << endl;
            cout << "para agregar automaticamente esta extension" << endl;
            cout << "1. si" << endl;
			cout << "2. no" << endl;
			cin >> op;
			if (op == 2) {
				queue<string> temp;
				while (!list.empty()) {
					temp.push((list.front() + ".PedroDorta"));
					list.pop();
				}
				list = temp;
			}

            

                while (!list.empty()) {
                    cout << "[arreglando :D]:" << list.front() << endl;
                    processFile(list.front(), delete_ext(list.front(), ".PedroDorta"), 5);
                    list.pop();
                }


            break;
        }
        default:
            cout << "Opción no válida. Intente de nuevo." << endl;

            break;
        }
    }


    return 0;
}