#define MODELLOADINGMAIN
#include "ModelLoading.h"


namespace StringEnum
{
	enum StringResolve { Invalid = -1, MYOBJ };
};


std::set<std::string> FileTypes = { "MYOBJ" };
std::map<std::string, int> ObjsLoaded;

LoadedModel ModelsLoaded[MAXMODELCOUNT];
unsigned int ModelLoadedCount = 0;




StringEnum::StringResolve EvaluateString(std::string Compare)
{
	if (Compare == "MYOBJ")
		return StringEnum::MYOBJ;

	return StringEnum::Invalid;
}

float*& ReturnVerticeCopy(unsigned int ID)
{
	float* NewVerts = new float[ModelsLoaded[ID].VertCount];
	for (int i = 0; i < ModelsLoaded[ID].VertCount; i++)
	{
		NewVerts[i] = ModelsLoaded[ID].Vertices[i];
	}
	return NewVerts;
}

unsigned int*& ReturnIndiceCopy(unsigned int ID)
{
	unsigned int* NewIndices = new unsigned int[ModelsLoaded[ID].IndiceCount];
	for (int i = 0; i < ModelsLoaded[ID].IndiceCount; i++)
	{
		NewIndices[i] = ModelsLoaded[ID].Indices[i];
	}
	return NewIndices;
}

void LoadModelCopy(Model& Model, unsigned int ID)
{
	Model.Vertices = ReturnVerticeCopy(ID);
	Model.Indices = ReturnIndiceCopy(ID);
	Model.VertCount = ModelsLoaded[ID].VertCount;
	Model.IndiceCount = ModelsLoaded[ID].IndiceCount;
	Model.Name = ModelsLoaded[ID].Name;
}

static unsigned int GetFreeModelSlot()
{
	for (unsigned int i = 0; i < MAXMODELCOUNT; i++)
	{
		if (ModelsLoaded[i].VertCount == 0)
			return i;
	}
	return 0;
}

static double ParseDouble(std::string String)
{
	bool Neg = false, Expo = false, Deci = false;
	double RetVal = 0, UseVal = 0, MulDivVal = 1;
	for (char const Char : String)
	{
		if (Char == '-')
		{
			Neg = true;
			continue;
		}
		if (Char == 'e' || Char == 'E')
		{
			Expo = true;
			RetVal += UseVal * MulDivVal;
			if (Neg)
				RetVal *= -1;
			Neg = false;
			Deci = false;
			UseVal = 0;
			continue;
		}
		if (Char == '.' && !Expo)
		{
			RetVal += UseVal;
			UseVal = 0;
			Deci = true;
			continue;
		}
		UseVal *= 10;
		UseVal += (Char - 48);
		if (Deci)
			MulDivVal /= 10;
	}
	if (Neg)
		UseVal *= -1;
	if (Expo)
	{
		RetVal *= std::pow(10, UseVal);
		return RetVal;
	}
	RetVal += UseVal * MulDivVal;
	return RetVal;
}

static unsigned int ParseUnsignedInterger(std::string String)
{
	unsigned int Ret = 0;
	for (const char Char : String)
	{
		Ret *= 10;
		Ret += Char - 48;
	}
	return Ret;
}

//Type of Element
static void ElementToArrayDrawing
(float*& OldVertices, unsigned int*& Indices, unsigned int IndiceCount, unsigned int TotalLength)
{
	float* Vertices = new float[IndiceCount * TotalLength];

	for (unsigned int Index = 0; Index < IndiceCount; Index++)
	{
		for (unsigned int i = 0; i < TotalLength; i++)
		{
			Vertices[Index * TotalLength + i] = OldVertices[Indices[Index] * TotalLength + i];
		}
	}
	delete[](OldVertices);
	OldVertices = Vertices;
}

//Type of Array
static void AddNormals
(float*& Vertices, unsigned int VertCount, unsigned int VertLength, unsigned int Padding)
{
	unsigned int TrueLength = VertLength + Padding;

	Vector3 Vert1;
	Vector3 Vert2;
	Vector3 Vert3;

	for (unsigned int i = 0; i < VertCount; i += TrueLength * 3)
	{
		Vert1.x = Vertices[i];
		Vert1.y = Vertices[i + 1];
		Vert1.z = Vertices[i + 2];

		Vert2.x = Vertices[i + TrueLength];
		Vert2.y = Vertices[i + TrueLength + 1];
		Vert2.z = Vertices[i + TrueLength + 2];

		Vert3.x = Vertices[i + (TrueLength * 2)];
		Vert3.y = Vertices[i + (TrueLength * 2) + 1];
		Vert3.z = Vertices[i + (TrueLength * 2) + 2];

		Vector3 Normal = CrossProduct(Vert2 - Vert1, Vert3 - Vert1);

		if (Padding > 0)
		{
			Vertices[i + VertLength] = Normal.x;
			Vertices[i + TrueLength + VertLength] = Normal.x;
			Vertices[i + (TrueLength * 2) + VertLength] = Normal.x;
		}
		if (Padding > 1)
		{
			Vertices[i + VertLength + 1] = Normal.y;
			Vertices[i + TrueLength + VertLength + 1] = Normal.y;
			Vertices[i + (TrueLength * 2) + VertLength + 1] = Normal.y;
		}
		if (Padding > 2)
		{
			Vertices[i + VertLength + 2] = Normal.z;
			Vertices[i + TrueLength + VertLength + 2] = Normal.z;
			Vertices[i + (TrueLength * 2) + VertLength + 2] = Normal.z;
		}
	}
}

//Type of Array
static void EdgeToRect
(float*& OldVertices, unsigned int &VertCount, unsigned int &IndiceCount, unsigned int NormalPos, unsigned int NormalLength, unsigned int TotalLength)
{
	float* Vertices = new float[VertCount * 4];
	IndiceCount *= 4;

	unsigned int EdgesIn = 0;
	for (unsigned int i = 0; i < VertCount * 4; i++)
	{
		if (i < VertCount)
			Vertices[i] = OldVertices[i];
		else
			Vertices[i] = 1;
	}

	List<Edge> Edges = 0;

	unsigned int ArrPos = 0;

	for (unsigned int i = 0; i < VertCount; i += TotalLength * 3)
	{
		for (int TriEdge = 0; TriEdge < 3; TriEdge++)
		{
			Vector3 Vert1;
			Vector3 Vert2;
			Vector3 Normal;
			if (NormalLength > 0)
				Normal.x = Vertices[i + NormalPos];
			if (NormalLength > 1)
				Normal.y = Vertices[i + NormalPos + 1];
			if (NormalLength > 2)
				Normal.z = Vertices[i + NormalPos + 2];

			switch (TriEdge)
			{
			case 0:
				Vert2 = { Vertices[i], Vertices[i + 1], Vertices[i + 2] };
				Vert1 = { Vertices[i + TotalLength], Vertices[i + TotalLength + 1], Vertices[i + TotalLength + 2] };
				break;
			case 1:
				Vert2 = { Vertices[i + TotalLength], Vertices[i + TotalLength + 1], Vertices[i + TotalLength + 2] };
				Vert1 = { Vertices[i + (2 * TotalLength)], Vertices[i + 1 + (2 * TotalLength)], Vertices[i + 2 + (2 * TotalLength)] };
				break;
			case 2:
				Vert2 = { Vertices[i + (2 * TotalLength)], Vertices[i + 1 + (2 * TotalLength)], Vertices[i + 2 + (2 * TotalLength)] };
				Vert1 = { Vertices[i], Vertices[i + 1], Vertices[i + 2] };
				break;
			}

			if (Edges.Contains({ Vert1, Vert2, Vector3(0) }, ArrPos))
			{
				Edge edge = *Edges[ArrPos];
				//Triangle 1
				Vertices[VertCount + (EdgesIn * TotalLength)] = Vert1.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = Vert1.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = Vert1.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = Normal.z;
				EdgesIn++;

				Vertices[VertCount + (EdgesIn * TotalLength)] = Vert2.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = Vert2.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = Vert2.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = Normal.z;
				EdgesIn++;

				Vertices[VertCount + (EdgesIn * TotalLength)] = edge.Vert1.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = edge.Vert1.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = edge.Vert1.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.z;
				EdgesIn++;


				//Triangle 2
				Vertices[VertCount + (EdgesIn * TotalLength)] = Vert1.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = Vert1.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = Vert1.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = Normal.z;
				EdgesIn++;

				Vertices[VertCount + (EdgesIn * TotalLength)] = edge.Vert1.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = edge.Vert1.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = edge.Vert1.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.z;
				EdgesIn++;

				Vertices[VertCount + (EdgesIn * TotalLength)] = edge.Vert2.x;
				Vertices[VertCount + 1 + (EdgesIn * TotalLength)] = edge.Vert2.y;
				Vertices[VertCount + 2 + (EdgesIn * TotalLength)] = edge.Vert2.z;
				if (NormalLength > 0)
					Vertices[VertCount + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.x;
				if (NormalLength > 1)
					Vertices[VertCount + 1 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.y;
				if (NormalLength > 2)
					Vertices[VertCount + 2 + NormalPos + (EdgesIn * TotalLength)] = edge.Normal.z;
				EdgesIn++;
			}
			else
				Edges.AddToList(Edge{ Vert1, Vert2, Normal });
		}
	}

	VertCount *= 4;
	Edges.Delete();
	delete[](OldVertices);
	OldVertices = Vertices;
}

static int LoadMYOBJFile(LoadedModel& CurrentModel, std::string ModelName, std::string FileType)
{
	unsigned int VertPadding = 3;

	//Load file
	std::string FileString;
	std::ifstream FileStream;
	unsigned int CharCount = 0;
	FileStream.exceptions(std::ios::failbit);
	try
	{
		FileStream.open("./Objects/" + ModelName + "." + FileType);
		std::stringstream Stream;
		Stream << FileStream.rdbuf();
		FileString = Stream.str();
		FileStream.seekg(0, std::ios::end);
		CharCount = FileStream.tellg();
		FileStream.close();
	}
	catch (const std::ios::failure& fail)
	{
		std::cout << "Failed to open " + ModelName + " file.";
	}

	//Get counts
	unsigned int VertCount = 0;
	unsigned int IndiceCount = 0;
	unsigned int StartingPoint = 0;
	std::string NumString;

	for (unsigned int Char = 0; Char < CharCount; Char++)
	{
		if ((FileString[Char] == ' ' && FileString[Char - 1] != ' ') || (FileString[Char] == '\r' && FileString[Char + 1] == '\n' && FileString[Char - 1] != '\n') || (FileString[Char] == '\n' && FileString[Char - 1] != '\n'))
		{
			if (VertCount == 0)
				VertCount = ParseUnsignedInterger(NumString) * 9;
			else
			{
				IndiceCount = ParseUnsignedInterger(NumString) * 3;
				StartingPoint = Char + 1;
				break;
			}
			NumString.clear();
		}
		if (FileString[Char] != ' ' && FileString[Char] != '\n' && FileString[Char] != '\r')
			NumString += FileString[Char];
	}

	VertCount += (VertCount / 9) * VertPadding;

	NumString.clear();

	//Get data
	float* Vertices = new float[VertCount];
	unsigned int* Indices = new unsigned int[IndiceCount];

	unsigned int VerticesLoaded = 0;
	unsigned int IndicesLoaded = 0;
	unsigned int CountInVert = 0;

	for (unsigned int Char = StartingPoint; Char < CharCount; Char++)
	{
		if (FileString[Char] == 'n')
			break;
		if ((FileString[Char] == ' ' && FileString[Char - 1] != ' ') || (FileString[Char] == '\r' && FileString[Char + 1] == '\n' && FileString[Char - 1] != '\n') || (FileString[Char] == '\n' && FileString[Char - 1] != '\n'))
		{
			if (VerticesLoaded >= VertCount && IndicesLoaded < IndiceCount)
			{
				Indices[IndicesLoaded] = ParseUnsignedInterger(NumString);
				IndicesLoaded++;
				NumString.clear();
				continue;
			}
			Vertices[VerticesLoaded] = ParseDouble(NumString);
			VerticesLoaded++;
			CountInVert++;
			if (CountInVert >= 9)
			{
				for (unsigned int i = 0; i < VertPadding; i++)
				{
					Vertices[VerticesLoaded + i] = 0;
				}
				VerticesLoaded += VertPadding;
				CountInVert = 0;
			}
			NumString.clear();
			continue;
		}
		if (FileString[Char] != ' ' && FileString[Char] != '\n' && FileString[Char] != '\r')
			NumString += FileString[Char];
	}
	Indices[IndicesLoaded] = ParseUnsignedInterger(NumString);

	//Return here for element-based drawing

	ElementToArrayDrawing(Vertices, Indices, IndiceCount, 9 + VertPadding);
	VertCount = IndiceCount * (9 + VertPadding);
	AddNormals(Vertices, VertCount, 9, VertPadding);

	if(ModelName == "Cube")
		EdgeToRect(Vertices, VertCount, IndiceCount, 9, 3, 12);

	//Debugging
	//for (int i = 0; i < VertCount; i++)
	//{
	//	std::cout << Vertices[i] << " ";
	//	if ((i + 1) % 12 == 0)
	//		std::cout << "\n";
	//	if ((i + 1) % 36 == 0)
	//		std::cout << "\n";
	//}

	CurrentModel.Vertices = Vertices;
	CurrentModel.Indices = Indices;
	CurrentModel.VertCount = VertCount;
	CurrentModel.IndiceCount = IndiceCount;
	CurrentModel.Name = ModelName;

	return 0;
}

static int LoadModelIntoBuffer(Model& CurrentModel, std::string ModelName, std::string FileType)
{
	if (CurrentModel.VertCount > 0)
		CurrentModel.DeleteModel();
	if (ObjsLoaded.contains(ModelName))
		return 0;

	if (ModelLoadedCount >= MAXMODELCOUNT)
		return 0;
	if (FileTypes.find(FileType) == FileTypes.end())
		return 0;
	if (!std::filesystem::is_regular_file("./Objects/" + ModelName + "." + FileType))
		return 0;

	unsigned int FreeModelSlot = GetFreeModelSlot();

	switch (EvaluateString(FileType))
	{
	case StringEnum::MYOBJ:
		LoadMYOBJFile(ModelsLoaded[FreeModelSlot], ModelName, FileType);
		break;
	case StringEnum::Invalid:
		break;
	default:
		break;
	}
	LoadModelCopy(CurrentModel, FreeModelSlot);
	ObjsLoaded.insert({ ModelName, FreeModelSlot });
	ModelLoadedCount++;
	return 0;
}

void UnloadModel(unsigned int ID)
{
	delete[](ModelsLoaded[ID].Vertices);
	ModelsLoaded[ID].Vertices = nullptr;
	delete[](ModelsLoaded[ID].Indices);
	ModelsLoaded[ID].Indices = nullptr;
	ModelsLoaded[ID].VertCount = 0;
	ModelsLoaded[ID].IndiceCount = 0;
	ModelLoadedCount--;
	ObjsLoaded.erase(ModelsLoaded[ID].Name);
}

void LoadModel(Model& CurrentModel, std::string ModelName, std::string FileType)
{
	if (CurrentModel.VertCount > 0)
		CurrentModel.DeleteModel();
	if (!ObjsLoaded.contains(ModelName))
	{
		LoadModelIntoBuffer(CurrentModel, ModelName, FileType);
		return;
	}
	LoadModelCopy(CurrentModel, ObjsLoaded[ModelName]);
}

namespace ModelLoading
{
	void DeleteHeap()
	{
		for (int i = 0; i < MAXMODELCOUNT; i++)
		{
			delete[](ModelsLoaded[i].Vertices);
			ModelsLoaded[i].Vertices = nullptr;
			delete[](ModelsLoaded[i].Indices);
			ModelsLoaded[i].Indices = nullptr;

			ModelsLoaded[i].VertCount = 0;
			ModelsLoaded[i].IndiceCount = 0;
		}
	}
}