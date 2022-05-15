#include <iostream>
#include <mpi.h>
#include <ctime>
#include <fstream>
#include <string>
#include <list>
using namespace std;

#define comm MPI_COMM_WORLD

//количество итераций жизненного цикла
const int maxIteration = 4;
//Код выхода
const int codeExit = -99;
//признак принтера А
const int signPrinterA = 11;
//признак принтера Б
const int signPrinterB = 21;
//признак принтера А и Б
const int signPrinterAandB = 31;
//признак конца печати на принтере А
const int exitPrintPrinterA = 111;
//признак конца печати на принтере Б
const int exitPrintPrinterB = 211;
//признак - можно печатать на принтере А 
const int canBePrintedA = 101;
//признак - можно печатать на принтере Б
const int canBePrintedB = 201;


/// <summary>
/// Функция определения времени и даты
/// </summary>
/// <returns>Строковое представление даты и времени</returns>
string GetDateAndTime()
{
	time_t now = time(0);

	char stringDate[100];
	ctime_s(stringDate, 100, &now);

	return stringDate;
}



/// <summary>
/// Определение необходима ли печать в данный момент времени
/// </summary>
/// <param name="rank">Номер процесса</param>
/// <returns>Необходима ли запись</returns>
bool NeedToPrint(int rank)
{
	srand(rank + time(NULL));
	//Степень необходимости печатать
	int gradePrint = rand() % 100 + 1;

	if (gradePrint > 43)
	{
		return true;
	}
	else
	{
		return false;
	}
}



/// <summary>
/// Генерация количества строк для печати на принтере
/// </summary>
/// <param name="rank">Номер процесса</param>
/// <param name="iteration">Номер итерации</param>
/// <returns>Количество строк для печати</returns>
int GenerateAmountStringPrint(int rank, int iteration)
{
	srand(rank + time(NULL) + iteration);
	//количество строк на печать
	int amountString = rand() % 21 + 5;
	amountString += iteration;
	return amountString;
}



/// <summary>
/// Генерация точки старта откуда начинать чтение книги
/// </summary>
/// <param name="rank">Номер процесса</param>
/// <param name="iteration">Номер итерации</param>
/// <returns>Точку старта чтения</returns>
int GenerateStartPointReadBook(int rank, int iteration)
{
	srand(rank + time(NULL) + iteration);
	//Указатель начала чтения книги
	int startPoint = rand() % 2201;
	startPoint += iteration;
	return startPoint;
}



/// <summary>
/// Класс для печати в файл-принтер
/// </summary>
class Printer
{
public:
	/// <summary>
	/// Метод считывания информации из книги
	/// </summary>
	/// <param name="pathName">Путь к книге</param>
	/// <param name="numberWordToRead">Номер слова откуда начать читать</param>
	/// <param name="amountStringToRead">Количество строк которые нужно прочитать</param>
	/// <returns>Возвращает прочитанную строку</returns>
	static string* ReadBook(string pathName, int numberWordToRead, int amountStringToRead)
	{
		ifstream file;
		//открытие файла
		file.open(pathName);

		//проверка открытия файла
		if (!file.is_open())
		{
			cout << "File not created" << endl;
			throw exception("file not created");
		}

		//сдвиг курсора для чтения
		file.seekg(numberWordToRead);
		//массив прочитанных строк
		string* msg = new string[amountStringToRead];

		//чтение из файла 
		for (size_t i = 0; i < amountStringToRead; i++)
		{
			getline(file, msg[i]);
		}

		//закрытие файла
		file.close();
		//запоминание прочитанного материала
		return msg;
	}


	/// <summary>
	/// Метод печати на принтере
	/// </summary>
	/// <param name="pathName">Путь к файлу для печати</param>
	/// <param name="amountStringToPrint">Количество строк для печати</param>
	/// <param name="messageToPrint">Сообщение для печати</param>
	/// /// <param name="rank">Номер пользователя</param>
	static void PrintingOnPrinter(string pathName, int amountStringToPrint, string* messageToPrint, int rank)
	{
		ofstream file;
		//открытие файла на добавление
		file.open(pathName, ios_base::app);

		//проверка открытия файла
		if (!file.is_open())
		{
			cout << "File not created" << endl;
			throw exception("file not created");
		}

		//чтение в файла из массива
		file << "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << endl;
		file << "\t\t " << GetDateAndTime() << "\t\t Написал пользователь под №" << rank << ":" << endl << endl;
		for (size_t i = 0; i < amountStringToPrint; i++)
		{
			file << messageToPrint[i] << endl;
		}

		//признак конца печати
		file << "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
		for (size_t i = 0; i < 15; i++)
		{
			file << endl;
		}

		//закрытие файл
		file.close();
	}
};



/// <summary>
/// Главная функция, где происходит параллельность
/// </summary>
int main(int argc, char** argv)
{
	int size, rank;

	if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
	{
		return 1;
	}
	if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)
	{
		MPI_Finalize();
		return 2;
	}
	if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
	{
		MPI_Finalize();
		return 3;
	}

	MPI_Status status;

	//роль сервера
	bool server = false;
	//клиент принтера А
	bool printerA = false;
	//клиент принтера Б
	bool printerB = false;
	//клиент обоих принтерев
	bool printerAandB = false;

	//пользователи только принтера А
	int amountOnlyPrinterA = size / 3;
	//пользователи только принтера Б
	int amountOnlyPrinterB = size / 3;
	//пользователи принтера А и Б
	int amountAllPrinters = size - (amountOnlyPrinterA + amountOnlyPrinterB + 1);

	//распределение ролей
	if (rank == 0)
	{
		server = true;
	}
	if ((rank > 0) && (rank <= amountOnlyPrinterA))
	{
		printerA = true;
	}
	if ((rank > amountOnlyPrinterA) && (rank <= (amountOnlyPrinterB + amountOnlyPrinterA)))
	{
		printerB = true;
	}
	if (rank > (amountOnlyPrinterB + amountOnlyPrinterA))
	{
		printerAandB = true;
	}

	//сервер
	if (server)
	{
		//признак нахождения сообщения
		int flagMessage = 0;
		//количество отправленных кодов выхода
		int amountExitCode = 0;

		//Признак занятости принтера А
		bool busyPrinterA = false;
		//Признак занятости принтера Б
		bool busyPrinterB = false;

		//очередь на печать принтера А
		list<int> queuePrinterA;
		//очередь на печать принтера Б
		list<int> queuePrinterB;

		//номер процесса который хочет печатать
		int rankSourceMessage;
		//номер процесса первого в очереди на печать принтера А
		int rankFirstQueuePrinterA;
		//номер процесса первого в очереди на печать принтера Б
		int rankFirstQueuePrinterB;

		//бесконечная работа сервера до выхода всех процессов
		while (true)
		{
			//работа с очередью
			//принтер А свободен и в очереди есть люди
			if ((!busyPrinterA) && (queuePrinterA.size() > 0))
			{
				//принтер занят
				busyPrinterA = true;
				//получение номера первого процесса в очереди на печать
				rankFirstQueuePrinterA = queuePrinterA.front();
				//выход первого процесса в очереди 
				queuePrinterA.pop_front();

				//предоставление доступа на печать на принтере А
				if (rankFirstQueuePrinterA > (amountOnlyPrinterB + amountOnlyPrinterA))
				{
					MPI_Send(&canBePrintedA, 1, MPI_INT, rankFirstQueuePrinterA, signPrinterAandB, comm);
				}
				else
				{
					MPI_Send(&canBePrintedA, 1, MPI_INT, rankFirstQueuePrinterA, signPrinterA, comm);
				}
				cout << "Пользователь №" << rankFirstQueuePrinterA << " начал печатать на принтере А" << endl;
			}
			//принтер Б свободен и в очереди есть люди
			if ((!busyPrinterB) && (queuePrinterB.size() > 0))
			{
				//принтер занят
				busyPrinterB = true;
				//получение номера первого процесса в очереди на печать
				rankFirstQueuePrinterB = queuePrinterB.front();
				//выход первого процесса в очереди 
				queuePrinterB.pop_front();

				//предоставление доступа на печать на принтере Б
				if (rankFirstQueuePrinterB > (amountOnlyPrinterB + amountOnlyPrinterA))
				{
					MPI_Send(&canBePrintedB, 1, MPI_INT, rankFirstQueuePrinterB, signPrinterAandB, comm);
				}
				else
				{
					MPI_Send(&canBePrintedB, 1, MPI_INT, rankFirstQueuePrinterB, signPrinterB, comm);
				}
				cout << "Пользователь №" << rankFirstQueuePrinterB << " начал печатать на принтере Б" << endl;
			}

			//ищем сообщения пользователя принтера А
			MPI_Iprobe(MPI_ANY_SOURCE, signPrinterA, comm, &flagMessage, &status);
			//пользователь принтера А хочет взаимодествовать с принтером
			if (flagMessage)
			{
				//получение желания пользователя на печать на принтере А
				MPI_Recv(&rankSourceMessage, 1, MPI_INT, status.MPI_SOURCE, signPrinterA, comm, &status);

				//Пользователь просит разрешение на печать
				if ((rankSourceMessage > 0) && (rankSourceMessage < size))
				{
					cout << "Пользователь №" << rankSourceMessage << " хочет печатать на принтере А" << endl;

					// принтер А свободен
					if (!busyPrinterA)
					{
						//принтер заняли
						busyPrinterA = true;
						//предоставление доступа на печать на принтере А
						MPI_Send(&canBePrintedA, 1, MPI_INT, rankSourceMessage, signPrinterA, comm);
						cout << "Пользователь №" << rankSourceMessage << " начал печатать на принтере А" << endl;
					}
					//принтер А занят печатью другого пользователя
					else
					{
						//добавление пользователя в очередь
						queuePrinterA.push_back(rankSourceMessage);
					}
				}
				//пользователь закончил печать на принтере А
				else if ((rankSourceMessage == exitPrintPrinterA))
				{
					//принтер свободен
					busyPrinterA = false;
					cout << "Пользователь №" << status.MPI_SOURCE << " закончил печатать на принтере А" << endl;
				}
				//у пользователей начались выходные - код выхода из программы
				else
				{
					amountExitCode++;
					cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
				}
			}

			//ищем сообщения пользователя принтера Б
			MPI_Iprobe(MPI_ANY_SOURCE, signPrinterB, comm, &flagMessage, &status);
			//пользователь принтера Б хочет взаимодествовать с принтером
			if (flagMessage)
			{
				//получение желания пользователя на печать на принтере Б
				MPI_Recv(&rankSourceMessage, 1, MPI_INT, status.MPI_SOURCE, signPrinterB, comm, &status);

				//Пользователь просит разрешение на печать
				if ((rankSourceMessage > 0) && (rankSourceMessage < size))
				{
					cout << "Пользователь №" << rankSourceMessage << " хочет печатать на принтере Б" << endl;

					// принтер Б свободен
					if (!busyPrinterB)
					{
						//принтер заняли
						busyPrinterB = true;
						//предоставление доступа на печать на принтере Б
						MPI_Send(&canBePrintedB, 1, MPI_INT, rankSourceMessage, signPrinterB, comm);
						cout << "Пользователь №" << rankSourceMessage << " начал печатать на принтере Б" << endl;
					}
					//принтер Б занят печатью другого пользователя
					else
					{
						//добавление пользователя в очередь
						queuePrinterB.push_back(rankSourceMessage);
					}
				}
				//пользователь закончил печать на принтере Б
				else if ((rankSourceMessage == exitPrintPrinterB))
				{
					//освобождение принтера
					busyPrinterB = false;
					cout << "Пользователь №" << status.MPI_SOURCE << " закончил печатать на принтере Б" << endl;
				}
				//у пользователей начались выходные - код выхода из программы
				else
				{
					amountExitCode++;
					cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
				}
			}

			//ищем сообщения пользователя любого принтера
			MPI_Iprobe(MPI_ANY_SOURCE, signPrinterAandB, comm, &flagMessage, &status);
			//пользователь любого принтера хочет взаимодействовать с принтером
			if (flagMessage)
			{
				//получение желания пользователя на печать на принтере Б
				MPI_Recv(&rankSourceMessage, 1, MPI_INT, status.MPI_SOURCE, signPrinterAandB, comm, &status);

				//Пользователь просит разрешение на печать
				if ((rankSourceMessage > 0) && (rankSourceMessage < size))
				{
					cout << "Пользователь №" << rankSourceMessage << " хочет печатать на любом принтере" << endl;

					// принтер А свободен
					if (!busyPrinterA)
					{
						//принтер заняли
						busyPrinterA = true;
						//предоставление доступа на печать на принтере А
						MPI_Send(&canBePrintedA, 1, MPI_INT, rankSourceMessage, signPrinterAandB, comm);
						cout << "Пользователь №" << rankSourceMessage << " начал печатать на принтере А" << endl;
					}
					// принтер Б свободен
					else if (!busyPrinterB)
					{
						//принтер заняли
						busyPrinterB = true;
						//предоставление доступа на печать на принтере Б
						MPI_Send(&canBePrintedB, 1, MPI_INT, rankSourceMessage, signPrinterAandB, comm);
						cout << "Пользователь №" << rankSourceMessage << " начал печатать на принтере Б" << endl;
					}
					//принтер А занят печатью другого пользователя
					else if (queuePrinterA.size() < queuePrinterB.size())
					{
						//добавление пользователя в очередь
						queuePrinterA.push_back(rankSourceMessage);
					}
					//принтер Б занят печатью другого пользователя
					else
					{
						//добавление пользователя в очередь
						queuePrinterB.push_back(rankSourceMessage);
					}
				}
				//пользователь закончил печать на принтере А
				else if ((rankSourceMessage == exitPrintPrinterA))
				{
					//освобождаем принтер
					busyPrinterA = false;
					cout << "Пользователь №" << status.MPI_SOURCE << " закончил печатать на принтере А" << endl;
				}
				//пользователь закончил печать на принтере Б
				else if ((rankSourceMessage == exitPrintPrinterB))
				{
					//освобождаем принтер
					busyPrinterB = false;
					cout << "Пользователь №" << status.MPI_SOURCE << " закончил печатать на принтере Б" << endl;
				}
				//у пользователей начались выходные - код выхода из программы
				else
				{
					amountExitCode++;
					cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
				}
			}

			//все пользователи ушли на выходные
			if (amountExitCode == size - 1)
			{
				//выключение сервера
				break;
			}
		}
	}
	//пользователи
	else
	{
		//прочитанное сообщение из книги
		string* readMessage;

		//количество строк на печать
		int amountStringToPrint;
		//точка старта откуда читать
		int startPointToRead;
		//принятое сообщение от сервера
		int receivedMessage;
		//номер итерации жизненного цикла
		int iteration = 0;

		//путь к файлу для чтения
		string pathFileRead;
		//путь к файлу для записи
		string pathFileWrite;

		//жизненный цикл
		for (; iteration < maxIteration; iteration++)
		{
			//пользователи принтера А
			if (printerA)
			{
				pathFileRead = "Books\\jokes.txt";
				pathFileWrite = "PrinterA.txt";
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//говорим серверу, что хотим печатать
					MPI_Send(&rank, 1, MPI_INT, 0, signPrinterA, comm);
					//получаем доступ к принтеру (ждем очередь)
					MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterA, comm, &status);

					//выбираем материал для печати
					readMessage = Printer::ReadBook(pathFileRead, startPointToRead, amountStringToPrint);
					//печатаем на принтере (в файл)
					Printer::PrintingOnPrinter(pathFileWrite, amountStringToPrint, readMessage, rank);

					//сообщение конца печати (выходим из кабинета)
					MPI_Send(&exitPrintPrinterA, 1, MPI_INT, 0, signPrinterA, comm);


					//очистка памяти
					delete[] readMessage;
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					MPI_Send(&codeExit, 1, MPI_INT, 0, signPrinterA, comm);
				}
			}
			//пользователи принтера Б
			if (printerB)
			{
				pathFileRead = "Books\\borodino.txt";
				pathFileWrite = "PrinterB.txt";
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//говорим серверу, что хотим печатать
					MPI_Send(&rank, 1, MPI_INT, 0, signPrinterB, comm);
					//получаем доступ к принтеру (ждем очередь)
					MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterB, comm, &status);

					//выбираем материал для печати
					readMessage = Printer::ReadBook(pathFileRead, startPointToRead, amountStringToPrint);
					//печатаем на принтере (в файл)
					Printer::PrintingOnPrinter(pathFileWrite, amountStringToPrint, readMessage, rank);

					//сообщение конца печати (выходим из кабинета)
					MPI_Send(&exitPrintPrinterB, 1, MPI_INT, 0, signPrinterB, comm);

					//очистка памяти
					delete[] readMessage;
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					MPI_Send(&codeExit, 1, MPI_INT, 0, signPrinterB, comm);
				}
			}
			//пользователи любого принтера 
			if (printerAandB)
			{
				pathFileRead = "Books\\remedy.txt";
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//говорим серверу, что хотим печатать
					MPI_Send(&rank, 1, MPI_INT, 0, signPrinterAandB, comm);
					//получаем доступ к принтеру (ждем очередь)
					MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterAandB, comm, &status);

					//проверяем на каком принтере печатаем
					if (receivedMessage == canBePrintedA)
					{
						pathFileWrite = "PrinterA.txt";
					}
					else
					{
						pathFileWrite = "PrinterB.txt";
					}

					//выбираем материал для печати
					readMessage = Printer::ReadBook(pathFileRead, startPointToRead, amountStringToPrint);
					//печатаем на принтере (в файл)
					Printer::PrintingOnPrinter(pathFileWrite, amountStringToPrint, readMessage, rank);

					//сообщение конца печати (выходим из кабинета)
					if (receivedMessage == canBePrintedA)
					{
						MPI_Send(&exitPrintPrinterA, 1, MPI_INT, 0, signPrinterAandB, comm);
					}
					else
					{
						MPI_Send(&exitPrintPrinterB, 1, MPI_INT, 0, signPrinterAandB, comm);
					}

					//очистка памяти
					delete[] readMessage;
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					MPI_Send(&codeExit, 1, MPI_INT, 0, signPrinterAandB, comm);
				}
			}
		}
	}
	MPI_Finalize();
	return 0;
}