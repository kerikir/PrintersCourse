#include <iostream>
#include <mpi.h>
#include <ctime>
#include <fstream>
#include <string>
#include <list>
using namespace std;

#define comm MPI_COMM_WORLD
//количество аргументов в сообщении серверу
#define SizeMessageServer 4

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

	if (gradePrint >= 25)
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
/// Тип данных для сообщения серверу
/// </summary>
struct MessageServer
{
	//номер процесса
	int numberProcess;
	//кол-во строк для печати
	int timeExecution;
	//место откуда начинаем чтение
	int startPoint;
	//имя файла откуда считывать
	char nameFile[20];
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
	if (size < 6)
	{
		if (rank == 0)
		{
			cout << "Количество процессов не соответствует требованию!" << endl;
		}
		MPI_Finalize();
		return 101;
	}

	MPI_Status status;

	//роль сервера
	bool server = false;
	//роль принтера А
	bool printerA = false;
	//роль принтера Б
	bool printerB = false;
	//клиент принтера А
	bool userPrinterA = false;
	//клиент принтера Б
	bool userPrinterB = false;
	//клиент обоих принтерев
	bool userPrinterAandB = false;

	//количество процессов не пользователей
	int amountNotUsers = 3;
	//пользователи только принтера А
	int amountOnlyPrinterA = size / 4;
	//пользователи только принтера Б
	int amountOnlyPrinterB = size / 4;
	//пользователи принтера А и Б
	int amountAllPrinters = size - (amountOnlyPrinterA + amountOnlyPrinterB + amountNotUsers);

	//сообщение серверу
	MessageServer messageServer;
	MPI_Datatype MsgServerType;
	MPI_Datatype MsgServerTypes[SizeMessageServer] =
	{
		MPI_INT,MPI_INT,MPI_INT,MPI_CHAR
	};
	int blockLengthMsgServer[SizeMessageServer] = { 1,1,1,20 };
	MPI_Aint dispMsgServer[SizeMessageServer];
	MPI_Aint temp;
	//получаем адреса
	MPI_Get_address(&messageServer.numberProcess, dispMsgServer);
	MPI_Get_address(&messageServer.timeExecution, dispMsgServer + 1);
	MPI_Get_address(&messageServer.startPoint, dispMsgServer + 2);
	MPI_Get_address(&messageServer.nameFile, dispMsgServer + 3);
	temp = dispMsgServer[0];
	for (int i = 0; i < SizeMessageServer; i++)
	{
		dispMsgServer[i] = dispMsgServer[i] - temp;
	}
	MPI_Type_create_struct(SizeMessageServer, blockLengthMsgServer, dispMsgServer, MsgServerTypes, &MsgServerType);
	//создаем тип
	MPI_Type_commit(&MsgServerType);

	//распределение ролей
	if (rank == 0)
	{
		server = true;
	}
	if (rank == 1)
	{
		printerA = true;
	}
	if (rank == 2)
	{
		printerB = true;
	}
	if ((rank >= amountNotUsers) && (rank < (amountOnlyPrinterA + amountNotUsers)))
	{
		userPrinterA = true;
	}
	if ((rank >= (amountOnlyPrinterA + amountNotUsers)) && (rank < (amountOnlyPrinterB + amountOnlyPrinterA + amountNotUsers)))
	{
		userPrinterB = true;
	}
	if (rank >= (amountOnlyPrinterB + amountOnlyPrinterA + amountNotUsers))
	{
		userPrinterAandB = true;
	}

	//признак нахождения сообщения
	int flagMessage = 0;

	//сервер
	if (server)
	{
		//количество отправленных кодов выхода
		int amountExitCode = 0;

		//Признак занятости принтера А
		bool busyPrinterA = false;
		//Признак занятости принтера Б
		bool busyPrinterB = false;

		//очередь на печать принтера А
		list<MessageServer> queuePrinterA;
		//очередь на печать принтера Б
		list<MessageServer> queuePrinterB;

		//информация о запросе
		MessageServer rankSourceMessage;
		//номер процесса первого в очереди на печать принтера А
		MessageServer rankFirstQueuePrinterA;
		//номер процесса первого в очереди на печать принтера Б
		MessageServer rankFirstQueuePrinterB;

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

				//отправка задачи-печати
				if (rankFirstQueuePrinterA.numberProcess >= (amountOnlyPrinterB + amountOnlyPrinterA + amountNotUsers))
				{
					MPI_Send(&rankFirstQueuePrinterA, 1, MsgServerType, 1, signPrinterAandB, comm);
				}
				else
				{
					MPI_Send(&rankFirstQueuePrinterA, 1, MsgServerType, 1, signPrinterA, comm);
				}
				cout << "Пользователь №" << rankFirstQueuePrinterA.numberProcess << " начал печатать на принтере А" << endl;
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

				//отправка задачи-печати
				if (rankFirstQueuePrinterB.numberProcess >= (amountOnlyPrinterB + amountOnlyPrinterA + amountNotUsers))
				{
					MPI_Send(&rankFirstQueuePrinterB, 1, MsgServerType, 2, signPrinterAandB, comm);
				}
				else
				{
					MPI_Send(&rankFirstQueuePrinterB, 1, MsgServerType, 2, signPrinterB, comm);
				}
				cout << "Пользователь №" << rankFirstQueuePrinterB.numberProcess << " начал печатать на принтере Б" << endl;
			}

			//ищем запросы
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flagMessage, &status);
			//пользователь принтера А хочет взаимодествовать с принтером
			if (flagMessage)
			{
				//отправил сообщение пользователь принтера А
				if (status.MPI_TAG == signPrinterA)
				{
					//получение желания пользователя на печать на принтере А
					MPI_Recv(&rankSourceMessage, 1, MsgServerType, status.MPI_SOURCE, signPrinterA, comm, &status);

					//принтер А закончил печать 
					if ((status.MPI_SOURCE == 1))
					{
						//отправка сообщения об окончании печати
						MPI_Send(&exitPrintPrinterA, 1, MPI_INT, rankSourceMessage.numberProcess, signPrinterA, comm);
						//принтер свободен
						busyPrinterA = false;

						cout << "Пользователь №" << rankSourceMessage.numberProcess << " закончил печатать на принтере А" << endl;
					}
					//Пользователь просит разрешение на печать
					else if ((rankSourceMessage.numberProcess >= amountNotUsers) && (rankSourceMessage.numberProcess < size))
					{
						cout << "Пользователь №" << rankSourceMessage.numberProcess << " хочет печатать на принтере А" << endl;

						// принтер А свободен
						if (!busyPrinterA)
						{
							//принтер заняли
							busyPrinterA = true;
							//предоставление доступа на печать на принтере А
							MPI_Send(&rankSourceMessage, 1, MsgServerType, 1, signPrinterA, comm);
							cout << "Пользователь №" << rankSourceMessage.numberProcess << " начал печатать на принтере А" << endl;
						}
						//принтер А занят печатью другого пользователя
						else
						{
							//добавление пользователя в очередь
							queuePrinterA.push_back(rankSourceMessage);
						}
					}
					//у пользователей начались выходные - код выхода из программы
					else if (rankSourceMessage.numberProcess == codeExit)
					{
						amountExitCode++;
						cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
					}
				}
				//отправил сообщение пользователь принтера Б
				if (status.MPI_TAG == signPrinterB)
				{
					///получение желания пользователя на печать на принтере А
					MPI_Recv(&rankSourceMessage, 1, MsgServerType, status.MPI_SOURCE, signPrinterB, comm, &status);

					//принтер Б закончил печать 
					if ((status.MPI_SOURCE == 2))
					{
						//отправка сообщения об окончании печати
						MPI_Send(&exitPrintPrinterB, 1, MPI_INT, rankSourceMessage.numberProcess, signPrinterB, comm);
						//принтер свободен
						busyPrinterB = false;

						cout << "Пользователь №" << rankSourceMessage.numberProcess << " закончил печатать на принтере Б" << endl;
					}
					//Пользователь просит разрешение на печать
					else if ((rankSourceMessage.numberProcess >= amountNotUsers) && (rankSourceMessage.numberProcess < size))
					{
						cout << "Пользователь №" << rankSourceMessage.numberProcess << " хочет печатать на принтере Б" << endl;

						// принтер Б свободен
						if (!busyPrinterB)
						{
							//принтер заняли
							busyPrinterB = true;
							//предоставление доступа на печать на принтере Б
							MPI_Send(&rankSourceMessage, 1, MsgServerType, 2, signPrinterB, comm);

							cout << "Пользователь №" << rankSourceMessage.numberProcess << " начал печатать на принтере Б" << endl;
						}
						//принтер Б занят печатью другого пользователя
						else
						{
							//добавление пользователя в очередь
							queuePrinterB.push_back(rankSourceMessage);
						}
					}
					//у пользователей начались выходные - код выхода из программы
					else if (rankSourceMessage.numberProcess == codeExit)
					{
						amountExitCode++;
						cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
					}
				}
				//отправил сообщение пользователь любого принтера
				if (status.MPI_TAG == signPrinterAandB)
				{
					//получение желания пользователя на печать на принтере А или Б
					MPI_Recv(&rankSourceMessage, 1, MsgServerType, status.MPI_SOURCE, signPrinterAandB, comm, &status);

					//пользователь закончил печать на принтере А
					if ((status.MPI_SOURCE == 1))
					{
						//отправка сообщения об окончании печати
						MPI_Send(&exitPrintPrinterA, 1, MPI_INT, rankSourceMessage.numberProcess, signPrinterAandB, comm);
						//освобождаем принтер
						busyPrinterA = false;
						cout << "Пользователь №" << rankSourceMessage.numberProcess << " закончил печатать на принтере А" << endl;
					}
					//пользователь закончил печать на принтере Б
					else if ((status.MPI_SOURCE == 2))
					{
						//отправка сообщения об окончании печати
						MPI_Send(&exitPrintPrinterB, 1, MPI_INT, rankSourceMessage.numberProcess, signPrinterAandB, comm);
						//освобождаем принтер
						busyPrinterB = false;
						cout << "Пользователь №" << rankSourceMessage.numberProcess << " закончил печатать на принтере Б" << endl;
					}
					//Пользователь просит разрешение на печать
					else if ((rankSourceMessage.numberProcess >= amountNotUsers) && (rankSourceMessage.numberProcess < size))
					{
						cout << "Пользователь №" << rankSourceMessage.numberProcess << " хочет печатать на любом принтере" << endl;

						// принтер А свободен
						if (!busyPrinterA)
						{
							//принтер заняли
							busyPrinterA = true;
							//предоставление доступа на печать на принтере А
							MPI_Send(&rankSourceMessage, 1, MsgServerType, 1, signPrinterAandB, comm);
							cout << "Пользователь №" << rankSourceMessage.numberProcess << " начал печатать на принтере А" << endl;
						}
						// принтер Б свободен
						else if (!busyPrinterB)
						{
							//принтер заняли
							busyPrinterB = true;
							//предоставление доступа на печать на принтере Б
							MPI_Send(&rankSourceMessage, 1, MsgServerType, 2, signPrinterAandB, comm);
							cout << "Пользователь №" << rankSourceMessage.numberProcess << " начал печатать на принтере Б" << endl;
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
					//у пользователей начались выходные - код выхода из программы
					else if (rankSourceMessage.numberProcess == codeExit)
					{
						amountExitCode++;
						cout << "Пользователь №" << status.MPI_SOURCE << " уходит на выходные" << endl;
					}
				}
			}

			//все пользователи ушли на выходные
			if (amountExitCode == size - amountNotUsers)
			{
				//выключение принтеров
				messageServer.numberProcess = codeExit;
				MPI_Send(&messageServer, 1, MsgServerType, 1, signPrinterA, comm);
				MPI_Send(&messageServer, 1, MsgServerType, 2, signPrinterB, comm);
				//выключение сервера
				break;
			}
		}
	}
	//принтеры 
	else if (printerA || printerB)
	{
		//имя файла куда печатать (наш принтер)
		string fileNameToWrite;
		if (printerA)
		{
			fileNameToWrite = "PrinterA.txt";
		}
		else
		{
			fileNameToWrite = "PrinterB.txt";
		}

		while (true)
		{
			//ищем задачу от сервера
			MPI_Iprobe(0, MPI_ANY_TAG, comm, &flagMessage, &status);
			//нашли сообщение
			if (flagMessage)
			{
				//получили запрос
				MPI_Recv(&messageServer, 1, MsgServerType, 0, status.MPI_TAG, comm, &status);
				//печатаем на сервер
				if (messageServer.numberProcess == codeExit)
				{
					break;
				}
				//печать на принтере
				else
				{
					//находим информацию для печати
					string* readingInfo = Printer::ReadBook(messageServer.nameFile, messageServer.startPoint, messageServer.timeExecution);
					//печатаем на принтере
					Printer::PrintingOnPrinter(fileNameToWrite, messageServer.timeExecution, readingInfo, messageServer.numberProcess);

					//сообщение серверу о конце печати
					MPI_Send(&messageServer, 1, MsgServerType, 0, status.MPI_TAG, comm);
				}
			}
		}
	}
	//пользователи
	else
	{
		//количество строк на печать
		int amountStringToPrint;
		//точка старта откуда читать
		int startPointToRead;
		//принятое сообщение от сервера
		int receivedMessage;

		//номер итерации жизненного цикла
		int iteration = 0;
		//количество распечатанного материала
		int amountPrintMaterial = 0;
		//количество материала которое нужно распечатать
		int amountWaitPrintMaterial = 0;

		//жизненный цикл
		for (; iteration < maxIteration; iteration++)
		{
			//пользователи принтера А
			if (userPrinterA)
			{
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//путь к файлу для чтения
					char pathFileRead[20] = "Books\\jokes.txt";

					amountWaitPrintMaterial++;
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//заполнение запроса
					messageServer.numberProcess = rank;
					messageServer.timeExecution = amountStringToPrint;
					messageServer.startPoint = startPointToRead;
					for (int i = 0; i < 20; i++)
					{
						messageServer.nameFile[i] = pathFileRead[i];

					}

					//отправка запроса на печать
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterA, comm);
					//ищем уведомление о конце печати
					MPI_Iprobe(0, signPrinterA, comm, &flagMessage, &status);
					//распечаталось
					if (flagMessage)
					{
						//берем распечатанный материал
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterA, comm, &status);
						amountPrintMaterial++;
					}
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					//ждем конца печати
					while (amountPrintMaterial != amountWaitPrintMaterial)
					{
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterA, comm, &status);
						amountPrintMaterial++;
					}
					//отправка информации о конце рабочей недели
					messageServer.numberProcess = codeExit;
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterA, comm);
				}
			}
			//пользователи принтера Б
			if (userPrinterB)
			{
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//путь к файлу для чтения
					char pathFileRead[20] = "Books\\borodino.txt";

					amountWaitPrintMaterial++;
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//заполнение запроса
					messageServer.numberProcess = rank;
					messageServer.timeExecution = amountStringToPrint;
					messageServer.startPoint = startPointToRead;
					for (int i = 0; i < 20; i++)
					{
						messageServer.nameFile[i] = pathFileRead[i];

					}

					//отправка запроса на печать
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterB, comm);
					//ищем уведомление о конце печати
					MPI_Iprobe(0, signPrinterB, comm, &flagMessage, &status);
					//распечаталось
					if (flagMessage)
					{
						//берем распечатанный материал
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterB, comm, &status);
						amountPrintMaterial++;
					}
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					//ждем конца печати
					while (amountPrintMaterial != amountWaitPrintMaterial)
					{
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterB, comm, &status);
						amountPrintMaterial++;
					}
					//отправка информации о конце рабочей недели
					messageServer.numberProcess = codeExit;
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterB, comm);
				}
			}
			//пользователи любого принтера 
			if (userPrinterAandB)
			{
				//определение нужно ли печатать
				if (NeedToPrint(rank))
				{
					//путь к файлу для чтения
					char pathFileRead[20] = "Books\\remedy.txt";

					amountWaitPrintMaterial++;
					//определение откуда начать чтение
					startPointToRead = GenerateStartPointReadBook(rank, iteration);
					//определение количества материала, которое нужно распечатать
					amountStringToPrint = GenerateAmountStringPrint(rank, iteration);

					//заполнение запроса
					messageServer.numberProcess = rank;
					messageServer.timeExecution = amountStringToPrint;
					messageServer.startPoint = startPointToRead;
					for (int i = 0; i < 20; i++)
					{
						messageServer.nameFile[i] = pathFileRead[i];

					}

					//отправка запроса на печать
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterAandB, comm);
					//ищем уведомление о конце печати
					MPI_Iprobe(0, signPrinterAandB, comm, &flagMessage, &status);
					//распечаталось
					if (flagMessage)
					{
						//берем распечатанный материал
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterAandB, comm, &status);
						amountPrintMaterial++;
					}
				}

				//смотрим на календарь (ждем конца недели)
				if (iteration == maxIteration - 1)
				{
					//ждем конца печати
					while (amountPrintMaterial != amountWaitPrintMaterial)
					{
						MPI_Recv(&receivedMessage, 1, MPI_INT, 0, signPrinterAandB, comm, &status);
						amountPrintMaterial++;
					}
					//отправка информации о конце рабочей недели
					messageServer.numberProcess = codeExit;
					MPI_Send(&messageServer, 1, MsgServerType, 0, signPrinterAandB, comm);
				}
			}
		}
	}

	//освобождение типа
	MPI_Type_free(&MsgServerType);
	MPI_Finalize();
	return 0;
}