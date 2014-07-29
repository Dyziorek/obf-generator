#include "stdafx.h"
#include <google\protobuf\descriptor.h>
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "Amenity.h"
#include "Street.h"
#include "targetver.h"

#include "iconv.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "MapRoutingTypes.h"
#include "RandomAccessFileWriter.h"
#include "BinaryMapDataWriter.h"
#include "ArchiveIO.h"


int BinaryMapDataWriter::OSMAND_STRUCTURE_INIT = 1;
int BinaryMapDataWriter:: MAP_INDEX_INIT = 2;
int BinaryMapDataWriter:: MAP_ROOT_LEVEL_INIT = 3;
int BinaryMapDataWriter:: MAP_TREE = 4;

int BinaryMapDataWriter:: ADDRESS_INDEX_INIT = 5;
int BinaryMapDataWriter:: CITY_INDEX_INIT = 6;

int BinaryMapDataWriter:: TRANSPORT_INDEX_INIT = 9;
int BinaryMapDataWriter:: TRANSPORT_STOPS_TREE = 10;
int BinaryMapDataWriter:: TRANSPORT_ROUTES = 11;

int BinaryMapDataWriter:: POI_INDEX_INIT = 12;
int BinaryMapDataWriter:: POI_BOX = 13;
int BinaryMapDataWriter:: POI_DATA = 14;
	
int BinaryMapDataWriter:: ROUTE_INDEX_INIT = 15;
int BinaryMapDataWriter:: ROUTE_TREE = 16;
int BinaryMapDataWriter:: ROUTE_BORDER_BOX = 17;

int BinaryMapDataWriter::SHIFT_COORDINATES = 5;
int BinaryMapDataWriter::MASK_TO_READ = ~((1 << SHIFT_COORDINATES) - 1);
int BinaryMapDataWriter::ROUTE_SHIFT_COORDINATES = 4;


BinaryMapDataWriter::BinaryMapDataWriter(RandomAccessFileWriter* outData) : dataOut(outData)
{
		raf = outData;
		raf->SetCodedOutStream(&dataOut);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndStructure::kVersionFieldNumber, 2, &dataOut);
		time_t timeDate;
		time(&timeDate);
		wfl::WireFormatLite::WriteInt64(obf::OsmAndStructure::kDateCreatedFieldNumber, timeDate, &dataOut);
		pushState(OSMAND_STRUCTURE_INIT);
}

BinaryMapDataWriter::~BinaryMapDataWriter(void)
{
}

void BinaryMapDataWriter::preserveInt32Size()
{
	__int64 local = getFilePointer();
	BinaryFileReference binRef = BinaryFileReference::createSizeReference(local);
	references.push_front(binRef);
	wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
}

int BinaryMapDataWriter::writeInt32Size()
{
	BinaryFileReference ref = references.front();
	references.pop_front();
	// write directly to file with shift et al...
	int length = ref.writeReference(*raf, getFilePointer());
	return length;
}

void BinaryMapDataWriter::writeRawVarint32(std::vector<uint8>& mapDataBuf,int toVarint32)
{
	//google::protobuf::uint32 result = wfl::WireFormatLite::ZigZagEncode32(toVarint32);
	while (toVarint32 > 0x7F)
	{
		mapDataBuf.push_back((static_cast<uint8>(toVarint32) & 0x7F) | 0x80);
		toVarint32 >>= 7;
	}
	mapDataBuf.push_back(static_cast<uint8>(toVarint32) & 0x7F);
}

bool BinaryMapDataWriter::writeStartMapIndex(std::string name)
{
	if (pushState(MAP_INDEX_INIT, OSMAND_STRUCTURE_INIT))
	{
		wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		//dataOut.WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber);
		preserveInt32Size();
		wfl::WireFormatLite::WriteString(obf::OsmAndMapIndex::kNameFieldNumber, name, &dataOut);
	}
	return true;
}

void BinaryMapDataWriter::writeMapEncodingRules(boost::ptr_map<std::string, MapRulType>& types) 
{
	peekState(MAP_INDEX_INIT);

	std::vector<MapRulType*> out;
		int highestTargetId = types.size();
		// 1. prepare map rule type to write
		for (auto rulType : types) {
			if (rulType.second->targetTagValue != nullptr || rulType.second->getFreq() == 0 || !rulType.second->isMap()) {
				rulType.second->setTargetId(highestTargetId++);
			} else {
				out.push_back(rulType.second);
			}
		}

		// 2. sort by frequency and assign ids
		std::sort(out.begin(), out.end(), [](MapRulType* op1, MapRulType* op2)
		{
			return op1->freq < op2->freq;
		});
		obf::OsmAndMapIndex_MapEncodingRule builderMap;

		for (int i = 0; i < out.size(); i++) {

			
			obf::OsmAndMapIndex_MapEncodingRule* builder = builderMap.New();
			
			MapRulType* rule = out[i];
			rule->setTargetId(i + 1);

			builder->set_tag(rule->getTag());
			if (rule->getValue() != "") {
				builder->set_value(rule->getValue());
			}
			builder->set_minzoom(rule->getMinzoom());
			if (rule->isAdditional()) {
				builder->set_type(1);
			} else if(rule->isText()) {
				builder->set_type(2);
			}
			builder->ByteSize();
			wfl::WireFormatLite::WriteMessage(obf::OsmAndMapIndex::kRulesFieldNumber, *builder, &dataOut);
			delete builder;
			//dataStream.writeMessage(OsmandOdb.OsmAndMapIndex.RULES_FIELD_NUMBER, rulet);
		}
	}


void BinaryMapDataWriter::startWriteMapLevelIndex(int minZoom, int maxZoom, int leftX, int rightX, int topY, int bottomY)
{
		pushState(MAP_ROOT_LEVEL_INIT, MAP_INDEX_INIT);

		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex::kLevelsFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();

		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber, maxZoom, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber, minZoom, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber, leftX, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kRightFieldNumber, rightX, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kTopFieldNumber, topY, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber, bottomY, &dataOut);
		

		stackBounds.push_front(Bounds(leftX, rightX, topY, bottomY));
	}

std::unique_ptr<BinaryFileReference> BinaryMapDataWriter::startMapTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, int landCharacteristic)
{
	int vacP[] = {MAP_ROOT_LEVEL_INIT, MAP_TREE};
	checkPeek(vacP, sizeof(vacP)/sizeof(int));
	if (states.front() == MAP_ROOT_LEVEL_INIT) {
		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		} else {
			wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		}
		pushState(MAP_TREE);
		preserveInt32Size();
		long fp = getFilePointer();

        // align with borders with grid
        leftX &= MASK_TO_READ;
        topY &= MASK_TO_READ;
		Bounds bounds = stackBounds.front();
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kLeftFieldNumber, leftX - bounds.left, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kRightFieldNumber,  rightX - bounds.right, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kTopFieldNumber, topY - bounds.top, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kBottomFieldNumber, bottomY - bounds.bottom, &dataOut);

		if(landCharacteristic != 0) {
			wfl::WireFormatLite::WriteBool(obf::OsmAndMapIndex_MapDataBox::kOceanFieldNumber, landCharacteristic < 0, &dataOut);
		}
		stackBounds.push_front(Bounds(leftX, rightX, topY, bottomY));
		std::unique_ptr<BinaryFileReference> ref;
		if (containsObjects) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber,  wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
			ref = std::move(std::unique_ptr<BinaryFileReference>(BinaryFileReference::createShiftReference(getFilePointer(), fp)));
			wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
		}
		return ref;
	}

void BinaryMapDataWriter::endWriteMapTreeElement(){
		popState(MAP_TREE);
		stackBounds.pop_front();
		writeInt32Size();
	}


obf::MapDataBlock* BinaryMapDataWriter::createWriteMapDataBlock(__int64 baseID)
{
	obf::MapDataBlock block;
	obf::MapDataBlock* pBlock = block.New();
	pBlock->set_baseid(baseID);
	return pBlock;
}

int BinaryMapDataWriter::skipSomeNodes(const void* coordinates, int len, int i, int x, int y, bool multi) {
		int delta;
		delta = 1;
		// keep first/latest point untouched
		// simplified douglas\peuker
		// just try to skip some points very close to this point
		while (i + delta < len - 1) {
			int nx = parseIntFromBytes(coordinates, (i + delta) * 8);
			int ny = parseIntFromBytes(coordinates, (i + delta) * 8 + 4);
			int nnx = parseIntFromBytes(coordinates, (i + delta + 1) * 8);
			int nny = parseIntFromBytes(coordinates, (i + delta + 1) * 8 + 4);
			if(nnx == 0 && nny == 0) {
				break;
			}
			double dist = BinaryMapDataWriter::orthogonalDistance(nx, ny, x, y, nnx, nny);
			if (dist > 31 ) {
				break;
			}
			delta++;
		}
		return delta;
	}

obf::MapData BinaryMapDataWriter::writeMapData(__int64 diffId, int pleft, int ptop, sqlite3_stmt* selectData, std::vector<int> typeUse,
			std::vector<int> addtypeUse, std::map<MapRulType, std::string>& names, std::unordered_map<std::string, int>& stringTable, obf::MapDataBlock* dataBlock,
			bool allowCoordinateSimplification)
			{
		obf::MapData data;
		// calculate size
		mapDataBuf.clear();
		int pcalcx = (pleft >> SHIFT_COORDINATES);
		int pcalcy = (ptop >> SHIFT_COORDINATES);
		const void* plData = sqlite3_column_blob(selectData, 1);
		int bloblSize = sqlite3_column_bytes(selectData, 1);
		int len = bloblSize / 8;
		int delta = 1;
		for (int i = 0; i < len; i+= delta) {
			int x = parseIntFromBytes(plData, i * 8);
			int y = parseIntFromBytes(plData, i * 8 + 4);
			int tx = (x >> SHIFT_COORDINATES) - pcalcx;
			int ty = (y >> SHIFT_COORDINATES) - pcalcy;
			writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(tx));
			writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(ty));
			pcalcx = pcalcx + tx ;
			pcalcy = pcalcy + ty ;
			delta = 1;
			if (allowCoordinateSimplification) {
				delta = skipSomeNodes(plData, len, i, x, y, false);
			}
		}
		//COORDINATES_SIZE += CodedOutputStream.computeRawVarint32Size(mapDataBuf.size())
		//		+ CodedOutputStream.computeTagSize(MapData.COORDINATES_FIELD_NUMBER) + mapDataBuf.size();
		int area = sqlite3_column_int(selectData, 0);
		if (area) {
			data.set_areacoordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
		} else {
			data.set_coordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
		}
		 plData = sqlite3_column_blob(selectData, 2);
		 bloblSize = sqlite3_column_bytes(selectData, 2);
		if (bloblSize) {
			mapDataBuf.clear();
			pcalcx = (pleft >> SHIFT_COORDINATES);
			pcalcy = (ptop >> SHIFT_COORDINATES);
			len = bloblSize / 8;
			for (int i = 0; i < len; i+= delta) {
				int x = parseIntFromBytes(plData, i * 8);
				int y = parseIntFromBytes(plData, i * 8 + 4);
				if (x == 0 && y == 0) {
					if (mapDataBuf.size() > 0) {
						data.add_polygoninnercoordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
						mapDataBuf.clear();
					}
					pcalcx = (pleft >> SHIFT_COORDINATES);
					pcalcy = (ptop >> SHIFT_COORDINATES);
				} else {
					int tx = (x >> SHIFT_COORDINATES) - pcalcx;
					int ty = (y >> SHIFT_COORDINATES) - pcalcy;

					writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(tx));
					writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(ty));

					pcalcx = pcalcx + tx ;
					pcalcy = pcalcy + ty ;
					delta = 1;
					if (allowCoordinateSimplification) {
						delta = skipSomeNodes(plData, len, i, x, y, true);
					}
				}
			}
		}
		
		mapDataBuf.clear();
		for (int i = 0; i < typeUse.size() ; i++) {
			writeRawVarint32(mapDataBuf, typeUse[i]);
		}
		data.set_types((void*)&mapDataBuf.front(), mapDataBuf.size());
		//TYPES_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapData.TYPES_FIELD_NUMBER)
		//		+ CodedOutputStream.computeRawVarint32Size(mapDataBuf.size()) + mapDataBuf.size();
		if (addtypeUse.size() > 0) {
			mapDataBuf.clear();
			for (int i = 0; i < addtypeUse.size() ; i++) {
				writeRawVarint32(mapDataBuf, addtypeUse[i]);
			}
			data.set_additionaltypes((void*)&mapDataBuf.front(), mapDataBuf.size());
			//TYPES_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapData.ADDITIONALTYPES_FIELD_NUMBER);
		}

		mapDataBuf.clear();
		if (names.size() > 0) {
			for (std::pair<MapRulType, std::string> s : names) {
				writeRawVarint32(mapDataBuf, s.first.getTargetId());
				int ls = 0;
				if (stringTable.find(s.second) == stringTable.end())
				{
					ls = stringTable.size();
					stringTable.insert(std::make_pair(s.second, ls));
				}
				else
				{
					ls = stringTable[s.second];
				}
				writeRawVarint32(mapDataBuf, ls);
			}
		}
		//STRING_TABLE_SIZE += mapDataBuf.size();
		if (names.size() > 0)
		{
			data.set_stringnames((void*)&mapDataBuf.front(), mapDataBuf.size());
		}

		data.set_id(diffId);
		//ID_SIZE += CodedOutputStream.computeSInt64Size(OsmandOdb.MapData.ID_FIELD_NUMBER, diffId);
		return data;
	}

	void BinaryMapDataWriter::writeMapDataBlock(obf::MapDataBlock* builder, std::unordered_map<std::string, int>& stringTable, BinaryFileReference& ref)
	 {
		 int vacP[] = {MAP_ROOT_LEVEL_INIT};
		checkPeek(vacP, sizeof(vacP)/sizeof(int));
		obf::StringTable bs;
		if (!stringTable.empty()) {
			std::list<std::pair<std::string, int>> sortList;
			for (std::pair<std::string,int> s : stringTable) {
				sortList.push_back(s);
			}

			sortList.sort([](std::pair<std::string, int> elemSort1, std::pair<std::string, int> elem2)
			{
				return elemSort1.second < elem2.second;
			});

			for (std::pair<std::string,int> s : sortList) {
				bs.add_s(s.first);
			}
		
		
		obf::StringTable* refSt = builder->mutable_stringtable();
		refSt->MergeFrom(bs);
		int size = refSt->ByteSize();

		//STRING_TABLE_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapDataBlock.STRINGTABLE_FIELD_NUMBER)
		//		+ CodedOutputStream.computeRawVarint32Size(size) + size;
		}
		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber,  wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE),&dataOut);
		
		//codedOutStream.flush();
		ref.writeReference(*raf, getFilePointer());
		//MapDataBlock block = builder.build();
		//MAP_DATA_SIZE += block.getSerializedSize();
		dataOut.WriteVarint32(builder->ByteSize());
		builder->SerializeToCodedStream(&dataOut);
		//codedOutStream.writeMessageNoTag(block);
	}

	void BinaryMapDataWriter::endWriteMapLevelIndex() {
		popState(MAP_ROOT_LEVEL_INIT);
		stackBounds.pop_front();
		int len = writeInt32Size();
		
		std::wstringstream strm;
		strm << L"Map Level writed size:";
		strm << " " << len << std::endl;
		OutputDebugString(strm.str().c_str());
		
		//log.info("MAP level SIZE : " + len);
	}

	void BinaryMapDataWriter::endWriteMapIndex()
	{
		popState(MAP_INDEX_INIT);
		int len = writeInt32Size();
		
		std::wstringstream strm;
		strm << L"Whole map writed size:";
		strm << " " << len << std::endl;
		OutputDebugString(strm.str().c_str());
		
	}

	bool BinaryMapDataWriter::startWriteAddressIndex(std::string name)
	{
		if (pushState(ADDRESS_INDEX_INIT, OSMAND_STRUCTURE_INIT))
		{
			wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kAddressIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
			preserveInt32Size();

			wfl::WireFormatLite::WriteString(obf::OsmAndAddressIndex::kNameFieldNumber, name, &dataOut);
			wfl::WireFormatLite::WriteString(obf::OsmAndAddressIndex::kNameEnFieldNumber, name, &dataOut);
		}
		return true;
	}
	
	void  BinaryMapDataWriter::endWriteAddressIndex()
	{
		popState(ADDRESS_INDEX_INIT);
		int len = writeInt32Size();
	#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"Address index writed size:";
		strm << " " << len << std::endl;
		OutputDebugString(strm.str().c_str());
	#endif
	}
	

	void BinaryMapDataWriter::startCityBlockIndex(int type)  {
		pushState(CITY_INDEX_INIT, ADDRESS_INDEX_INIT);
		wfl::WireFormatLite::WriteTag(obf::OsmAndAddressIndex::kCitiesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		preserveInt32Size();
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber, type, &dataOut);
	}

	void BinaryMapDataWriter::endCityBlockIndex()  {
		popState(CITY_INDEX_INIT);
		int length = writeInt32Size();
	#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"City Block index writed size:";
		strm << " " << length << std::endl;
		OutputDebugString(strm.str().c_str());
	#endif
	}

	std::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> BinaryMapDataWriter::writeIndexedTable(int tag, std::list<std::string> indexedTable)  {
		wfl::WireFormatLite::WriteTag(tag, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();
		std::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> res;
		long init = getFilePointer();
		for (std::string e : indexedTable) {
			wfl::WireFormatLite::WriteString(obf::IndexedStringTable::kKeyFieldNumber, e, &dataOut);
			wfl::WireFormatLite::WriteTag(obf::IndexedStringTable::kValFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
			BinaryFileReference* ref = BinaryFileReference::createShiftReference(getFilePointer(), init);
			wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
			res.insert(std::make_pair(e, std::shared_ptr<BinaryFileReference>(ref)));
		}
		writeInt32Size();
		return res;
	}

	void BinaryMapDataWriter::writeAddressNameIndex(std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex){
		int peeker[] = {ADDRESS_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		wfl::WireFormatLite::WriteTag(obf::OsmAndAddressIndex::kNameIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();
		 std::list<std::string> namesIndexKeys;
		 for (auto pairData : namesIndex)
		 {
			 namesIndexKeys.push_back(pairData.first);
		 }
		
		 std::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> res = writeIndexedTable(obf::OsmAndAddressNameIndexData::kTableFieldNumber, namesIndexKeys);
		for(auto entry : namesIndex) {
			std::shared_ptr<BinaryFileReference> ref = res[entry.first];
			wfl::WireFormatLite::WriteTag(obf::OsmAndAddressNameIndexData::kAtomFieldNumber, wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE), &dataOut);
			long pointer = getFilePointer();
			if(ref != nullptr) {
				ref->writeReference(*raf, getFilePointer());
			}
			obf::OsmAndAddressNameIndexData_AddressNameIndexData builder;
			// collapse same name ?
			for(std::shared_ptr<MapObject> om : entry.second){
				obf::AddressNameIndexDataAtom atom;
				
				// this is optional
//				atom.setName(o.getName());
//				if(checkEnNameToWrite(o)){
//					atom.setNameEn(o.getEnName());
//				}
				int type = 1;
				std::shared_ptr<CityObj> oc = std::dynamic_pointer_cast<CityObj>(om);
				std::shared_ptr<Street> os = std::dynamic_pointer_cast<Street>(om);
				if (oc ) {
					if (oc->getType() == "") {
						type = 2;
					} else {
						std::string ct = oc->getType();
						if (ct != "city" && ct != "town") {
							type = 3;
						}
					}
				} else if(os) {
					type = 4;
				}
				atom.set_type(type); 
				atom.add_shifttocityindex((int) (pointer - om->getFileOffset()));
				if(os){
					atom.add_shifttocityindex((int) (pointer - os->getCity().getFileOffset()));
				}
				builder.add_atom()->MergeFrom(atom);

			}
			builder.SerializeToCodedStream(&dataOut);
		}

		int len = writeInt32Size();
	#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"City Adress Name Block index writed size:";
		strm << " " << len << std::endl;
		OutputDebugString(strm.str().c_str());
	#endif
	}

	bool BinaryMapDataWriter::checkEnNameToWrite(MapObject& obj) {
		if (obj.getEnName() == "" ) {
			return false;
		}
		return !(obj.getEnName() == obj.getName());
	}

	BinaryFileReference* BinaryMapDataWriter::writeCityHeader(MapObject& city, int cityType)
	{
		int peeker[] = {CITY_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		wfl::WireFormatLite::WriteTag(obf::OsmAndAddressIndex_CitiesIndex::kCitiesFieldNumber, wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE), &dataOut);
		
		long startMessage = getFilePointer();
		
		
		obf::CityIndex cityInd;
		if(cityType >= 0) {
			cityInd.set_city_type(cityType);
		}
		if(city.getID() != -1) {
			cityInd.set_id(city.getID());
		}
		
		cityInd.set_name(city.getName());
		if(checkEnNameToWrite(city)){
			iconverter ic("UTF-8", "ASCII");
			cityInd.set_name_en(ic.convert(city.getEnName()));
		}
		int cx = MapUtils::get31TileNumberX(city.getLatLon().second);
		int cy = MapUtils::get31TileNumberY(city.getLatLon().first);
		cityInd.set_x(cx);
		cityInd.set_y(cy);
		cityInd.set_shifttocityblockindex(0);
		cityInd.SerializeToCodedStream(&dataOut);
		return BinaryFileReference::createShiftReference(getFilePointer() - 4, startMessage);
		
	}
	
 obf::StreetIndex BinaryMapDataWriter::createStreetAndBuildings(Street street, int cx, int cy, std::string postcodeFilter, 
			std::unordered_map<__int64,std::set<Street>>& mapNodeToStreet, std::unordered_map<Street, std::list<EntityNode>>& wayNodes){
		int peeker[] = {CITY_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		 obf::StreetIndex streetBuilder;
		streetBuilder.set_name(street.getName());
		if (checkEnNameToWrite(street)) {
			iconverter ic("UTF-8", "ASCII");
			streetBuilder.set_name_en(ic.convert(street.getEnName()));
		}
		streetBuilder.set_id(street.getID());

		int sx = MapUtils::get31TileNumberX(street.getLatLon().second);
		int sy = MapUtils::get31TileNumberY(street.getLatLon().first);
		streetBuilder.set_x((sx - cx) >> 7);
		streetBuilder.set_y((sy - cy) >> 7);

		street.sortBuildings();
		for (Building b : street.getBuildings()) {
			if (postcodeFilter != "" && !  boost::iequals(postcodeFilter,b.postCode)) {
				continue;
			}
			obf::BuildingIndex  bbuilder;
			int bx = MapUtils::get31TileNumberX(b.getLatLon().second);
			int by = MapUtils::get31TileNumberY(b.getLatLon().first);
			bbuilder.set_x((bx - sx) >> 7);
			bbuilder.set_x((by - sy) >> 7);
			
			std::string number2 = b.getName2();
			if(number2 != ""){
				LatLon loc = b.getLatLon2();
				if(loc.first == -1000) {
					bbuilder.set_x((bx - sx) >> 7);
					bbuilder.set_y((by - sy) >> 7);
				} else {
					int bcx = MapUtils::get31TileNumberX(loc.second);
					int bcy = MapUtils::get31TileNumberY(loc.first);
					bbuilder.set_x2((bcx - sx) >> 7);
					bbuilder.set_y2((bcy - sy) >> 7);
				}
				bbuilder.set_name2(number2);
				if(b.interpType != Building::BuildingInterpolation::NONE) {
					bbuilder.set_interpolation(b.getInterpValue());
				} else if(b.interval > 0) {
					bbuilder.set_interpolation(b.interval);
				} else {
					bbuilder.set_interpolation(1);
				}
			}
			bbuilder.set_id(b.getID());
			bbuilder.set_name(b.getName());
			if (checkEnNameToWrite(b)) {
				iconverter ic("UTF-8", "ASCII");
				bbuilder.set_name_en(ic.convert(b.getEnName()));
			}
			if (postcodeFilter == "" && b.postCode != "") {
				bbuilder.set_postcode(b.postCode);
			}
			streetBuilder.add_buildings()->MergeFrom(bbuilder);
		}

		if(!wayNodes.empty()) {
			std::unordered_set<Street> checkedStreets ;
			for (EntityNode intersection : wayNodes[street]) {
				for (Street streetJ : mapNodeToStreet[intersection.id]) {
					if (checkedStreets.find(streetJ) != checkedStreets.end() || streetJ.getID() == street.getID()) {
						continue;
					}
					checkedStreets.insert(streetJ);
					obf::StreetIntersection  builder;
					int ix = MapUtils::get31TileNumberX(intersection.getLatLon().second);
					int iy = MapUtils::get31TileNumberY(intersection.getLatLon().first);
					builder.set_intersectedx((ix - sx) >> 7);
					builder.set_intersectedy((iy - sy) >> 7);
					builder.set_name(streetJ.getName());
					if(checkEnNameToWrite(streetJ)){
						iconverter ic("UTF-8", "ASCII");
						builder.set_name_en(ic.convert(streetJ.getEnName()));
					}
					streetBuilder.add_intersections()->MergeFrom(builder);
				}
			}
		}

		return streetBuilder;
	}


	void BinaryMapDataWriter::writeCityIndex(CityObj cityOrPostcode, std::list<Street>& streets, std::unordered_map<Street, std::list<EntityNode>>& wayNodes, 
			BinaryFileReference *ref)  {
		int peeker[] = {CITY_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		
		wfl::WireFormatLite::WriteTag(obf::OsmAndAddressIndex_CitiesIndex::kBlocksFieldNumber, wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE), &dataOut);
		long startMessage = getFilePointer();
		long startCityBlock = ref->getStartPointer();
		ref->writeReference(*raf, startMessage);
		obf::CityBlockIndex cityInd;
		cityInd.set_shifttocityindex((int) (startMessage - startCityBlock));
		long currentPointer = startMessage + 4 + wfl::WireFormatLite::TagSize(obf::CityBlockIndex::kShiftToCityIndexFieldNumber, wfl::WireFormatLite::FieldType::TYPE_INT32);
		
		int cx = MapUtils::get31TileNumberX(cityOrPostcode.getLatLon().second);
		int cy = MapUtils::get31TileNumberY(cityOrPostcode.getLatLon().first);
		std::unordered_map<__int64,std::set<Street>> mapNodeToStreet;
		if (!wayNodes.empty()) {
			for (std::list<Street>::iterator it = streets.begin() ; it != streets.end(); it++) {
				for (EntityNode n : wayNodes[*it]) {
					if (mapNodeToStreet.find(n.id) == mapNodeToStreet.end()) {
						mapNodeToStreet.insert(std::make_pair(n.id, std::set<Street>()));
					}
					mapNodeToStreet[n.id].insert(*it);
				}
			}
		}
		std::string postcodeFilter = cityOrPostcode.isPostcode() ? cityOrPostcode.getName() : "";
		for (Street s : streets) {
			obf::StreetIndex streetInd = createStreetAndBuildings(s, cx, cy, postcodeFilter, mapNodeToStreet, wayNodes);
			currentPointer += wfl::WireFormatLite::TagSize(obf::CityBlockIndex::kStreetsFieldNumber, wfl::WireFormatLite::FieldType::TYPE_INT32);
			if(currentPointer > INT_MAX) {
				throw new std::bad_exception("File size > 2GB");
			}
			s.setFileOffset((int) currentPointer);
			streetInd.ByteSize();
			currentPointer +=  wfl::WireFormatLite::MessageSize(streetInd);
			cityInd.add_streets()->MergeFrom(streetInd);
			
		}
		int size = wfl::WireFormatLite::UInt32Size(wfl::WireFormatLite::MessageSize(cityInd));
		cityInd.SerializeToCodedStream(&dataOut);
		for (Street s : streets) {
			s.setFileOffset(s.getFileOffset() + size);
		}
	}

	__int64 BinaryMapDataWriter::startWritePoiIndex(std::string name, int left31, int right31, int bottom31, int top31)
	{
		pushState(POI_INDEX_INIT, OSMAND_STRUCTURE_INIT);
		wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kPoiIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		stackBounds.push_front(Bounds(0,0,0,0));
		preserveInt32Size();
		__int64 fpp = getFilePointer();
		if (name != "")
		{
			wfl::WireFormatLite::WriteString(obf::OsmAndPoiIndex::kNameFieldNumber, name, &dataOut);
		}

		obf::OsmAndTileBox box;
		box.set_bottom(bottom31);
		box.set_left(left31);
		box.set_right(right31);
		box.set_top(top31);
		box.ByteSize();
		wfl::WireFormatLite::WriteMessage(obf::OsmAndPoiIndex::kBoundariesFieldNumber, box, &dataOut);
		return fpp;
	}

	void BinaryMapDataWriter::endWritePoiIndex(){
		popState(POI_INDEX_INIT);
		int len = writeInt32Size();
		stackBounds.pop_front();
		
	}

	void BinaryMapDataWriter::writePoiCategoriesTable(POICategory& cs){
		int peeker[] = {POI_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));

		int i = 0;
		for (auto cat : cs.categories) {
			obf::OsmAndCategoryTable builder;
			builder.set_category(cat.first);
			std::set<std::string> subcatSource = cs.categories[cat.first];
			cs.catIndexes[cat.first] =  i;
			int j = 0;
			for (std::string s : subcatSource) {
				cs.catSubIndexes[cat.first + char(-1) + s] = j;
				builder.add_subcategories(s);
				j++;
			}
			builder.ByteSize();
			wfl::WireFormatLite::WriteMessage(obf::OsmAndPoiIndex::kCategoriesTableFieldNumber, builder, &dataOut);
			i++;
		}

	}
	
	void BinaryMapDataWriter::writePoiSubtypesTable(POICategory& cs) {
		int peeker[] = {POI_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));

		int subcatId = 0;
		
		std::unordered_map<std::string, std::vector<MapRulType*>> groupAdditionalByTagName;
		for(MapRulType* rt : cs.attributes) {
			if(rt->isAdditional()) {
				if(groupAdditionalByTagName.find(rt->getTag()) == groupAdditionalByTagName.end()) {
					groupAdditionalByTagName.insert(std::make_pair(rt->getTag(), std::vector<MapRulType*>()));
				}
				groupAdditionalByTagName[rt->getTag()].push_back(rt);
			} else {
				rt->setTargetPoiId(subcatId++, 0);
			}
		}
		
		for(auto addTag : groupAdditionalByTagName) {
			int cInd = subcatId++;
			std::vector<MapRulType*> list = addTag.second;
			
			int subcInd = 0;
			for(MapRulType* subtypeVal :  list){
				subtypeVal->setTargetPoiId(cInd, subcInd++);
			}
		}
	}
	
	void BinaryMapDataWriter::writePoiCategories(POICategory& poiCats){
		int peeker[] = {POI_BOX};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		
		obf::OsmAndPoiCategories builder;
		int prev = -1;
		std::sort(poiCats.cachedCategoriesIds.begin(), poiCats.cachedCategoriesIds.end(), std::less<int>());
		for (std::vector<int>::iterator it = poiCats.cachedCategoriesIds.begin(); it != poiCats.cachedCategoriesIds.end(); it++) {
			// avoid duplicates
			if (it == poiCats.cachedCategoriesIds.begin() || prev != *it) {
				builder.add_categories(*it);
				prev = *it;
			}
		}
		builder.ByteSize();
		wfl::WireFormatLite::WriteMessage(obf::OsmAndPoiBox::kCategoriesFieldNumber, builder, &dataOut);
	}

	 std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>> BinaryMapDataWriter::writePoiNameIndex(std::unordered_map<std::string, std::unordered_set<POIBox>>& namesIndex, __int64 startPoiIndex){
		int peeker[] = {POI_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));

		wfl::WireFormatLite::WriteTag(obf::OsmAndPoiIndex::kNameIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();
		
		std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>> fpToWriteSeeks;
		std::list<std::string> names;
		auto itMap = namesIndex.begin();
		std::transform(namesIndex.begin(), namesIndex.end(), std::back_inserter(names), std::bind(&std::unordered_map<std::string, std::unordered_set<POIBox>>::value_type::first, std::placeholders::_1));
		std::unordered_map<std::string,  std::shared_ptr<BinaryFileReference>> indexedTable = writeIndexedTable(obf::OsmAndPoiNameIndex::kTableFieldNumber, names);
		for(auto e : namesIndex) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndPoiNameIndex::kDataFieldNumber, wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE), &dataOut);
			std::shared_ptr<BinaryFileReference> nameTableRef = indexedTable[e.first];
			nameTableRef->writeReference(*raf, getFilePointer());
			
			obf::OsmAndPoiNameIndex_OsmAndPoiNameIndexData builder;
			std::unordered_set<POIBox>& tileBoxes = e.second;
			std::list<std::pair<POIBox,int>> tileBoxSizes;
			for(POIBox box : tileBoxes) {
				obf::OsmAndPoiNameIndexDataAtom bs;
				bs.set_x(box.x);
				bs.set_y(box.y);
				bs.set_zoom(box.zoom);
				bs.set_shiftto(0);
				bs.ByteSize();
				builder.add_atoms()->MergeFrom(bs);
				tileBoxSizes.push_back(std::make_pair(box, bs.ByteSize()));
			}
			
			builder.ByteSize();
			builder.SerializeToCodedStream(&dataOut);
			long endPointer = getFilePointer();
			
			// first message
			int accumulateSize = 4;
			for (std::list<std::pair<POIBox,int>>::reverse_iterator i = tileBoxSizes.rbegin(); i != tileBoxSizes.rend(); i++) {
				POIBox box =i->first;
				if (fpToWriteSeeks.find(box) == fpToWriteSeeks.end()) {
					fpToWriteSeeks.insert(std::make_pair(box, std::list<std::shared_ptr<BinaryFileReference>>()));
				}
				fpToWriteSeeks[box].push_back(std::shared_ptr<BinaryFileReference>(BinaryFileReference::createShiftReference(endPointer - accumulateSize, startPoiIndex)));
				int tagSize = wfl::WireFormatLite::TagSize(obf::OsmAndPoiNameIndex_OsmAndPoiNameIndexData::kAtomsFieldNumber, wfl::WireFormatLite::FieldType::TYPE_INT32);
				accumulateSize += tagSize + i->second;

			}
		}
		
		writeInt32Size();
		
		
		return fpToWriteSeeks;
	}

	

	 void BinaryMapDataWriter::writePoiDataAtom(long id, int x24shift, int y24shift, 
			std::string type, std::string subtype,  std::unordered_map<MapRulType*, std::string>& additionalNames, OBFRenderingTypes& rtypes, 
			POICategory&  globalCategories){
		int peeker[] = {POI_DATA};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		std::vector<int> types = globalCategories.cachedCategoriesIds;
		obf::OsmAndPoiBoxDataAtom builder;
		builder.set_dx(x24shift);
		builder.set_dy(y24shift);
		for (int i = 0; i < types.size(); i++) {
			int j = types[i];
			builder.add_categories(j);
		}
		
		builder.set_id(id);

		if (/*USE_DEPRECATED_POI_NAME_STRUCTURE*/ true) {
			std::string name = "";
			for(auto addName : additionalNames)
			{
				if (addName.first->getInternalId() == rtypes.nameRule->getInternalId())
				{
					std::string name = addName.second;
					additionalNames.erase(addName.first);
					break;
				}
			}
			if (name != "") {
				builder.set_name(name);
			}

			std::string nameEn = "";
			for(auto addName : additionalNames)
			{
				if (addName.first->getInternalId() == rtypes.nameEnRule->getInternalId())
				{
					std::string name = addName.second;
					additionalNames.erase(addName.first);
					break;
				}
			}

			if (nameEn != "") {
				builder.set_nameen(nameEn);
			}	
		}
		
		if (/*USE_DEPRECATED_POI_NAME_ADD_INFO_STRUCTURE*/ true ) {
			std::string openingHours = "";
			std::string site =  "";
			std::string phone =  "";
			std::string description =  "";


			for(auto addName = additionalNames.begin(); addName != additionalNames.end(); addName++)
			{
				if (addName->first->getInternalId() == rtypes.getRuleType("opening_hours", "", true)->getInternalId())
				{
					openingHours = addName->second;
					addName = additionalNames.erase(addName);
					if (addName == additionalNames.end())
					{
						// erased last item go away
						break;
					}
				}
				if (addName->first->getInternalId() == rtypes.getRuleType("website", "", true)->getInternalId())
				{
					site = addName->second;
					addName = additionalNames.erase(addName);
					if (addName == additionalNames.end())
					{
						// erased last item go away
						break;
					}
				}
				if (addName->first->getInternalId() == rtypes.getRuleType("phone", "", true)->getInternalId())
				{
					phone = addName->second;
					addName = additionalNames.erase(addName);
					if (addName == additionalNames.end())
					{
						// erased last item go away
						break;
					}
				}
				if (addName->first->getInternalId() == rtypes.getRuleType("description", "", true)->getInternalId())
				{
					description = addName->second;
					addName = additionalNames.erase(addName);
				}
				if (addName == additionalNames.end())
				{
					// erased last item go away
					break;
				}
			}
			if (openingHours != "") {
				builder.set_openinghours(openingHours);
			}
			if (site != "") {
				builder.set_site(site);
			}
			if (phone!="") {
				builder.set_phone(phone);
			}
			if (description!="") {
				builder.set_note(description);
			}
		}
		builder.ByteSize();

		wfl::WireFormatLite::WriteMessage(obf::OsmAndPoiBoxData::kPoiDataFieldNumber, builder, &dataOut);

	}

	 void BinaryMapDataWriter::startWritePoiData(int zoom, int x, int y, std::vector<std::shared_ptr<BinaryFileReference>>& fpPoiBox){
		pushState(POI_DATA, POI_INDEX_INIT);
		wfl::WireFormatLite::WriteTag(obf::OsmAndPoiIndex::kPoiDataFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		__int64 pointer = getFilePointer();
		preserveInt32Size();
		// write shift to that data
		for (int i = 0; i < fpPoiBox.size(); i++) {
			fpPoiBox[i]->writeReference(*raf, pointer);
		}

		wfl::WireFormatLite::WriteUInt32(obf::OsmAndPoiBoxData::kZoomFieldNumber, zoom, &dataOut);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndPoiBoxData::kXFieldNumber, x, &dataOut);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndPoiBoxData::kYFieldNumber, y, &dataOut);

	}

	 void BinaryMapDataWriter::endWritePoiData(){
		popState(POI_DATA);
		writeInt32Size();
	}

	 std::shared_ptr<BinaryFileReference> BinaryMapDataWriter::startWritePoiBox(int zoom, int tileX, int tileY, __int64 startPoiIndex, bool end)
	{
		int peeker[] = {POI_INDEX_INIT, POI_BOX};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		if (states.front() == POI_INDEX_INIT) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndPoiIndex::kBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		} else {
			wfl::WireFormatLite::WriteTag(obf::OsmAndPoiBox::kSubBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		}
		pushState(POI_BOX);
		preserveInt32Size();

		Bounds bounds = stackBounds.front();
		int parentZoom = bounds.right;
		int parentTileX = bounds.left;
		int parentTileY = bounds.top;

		int pTileX = parentTileX << (zoom - parentZoom);
		int pTileY = parentTileY << (zoom - parentZoom);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndPoiBox::kZoomFieldNumber, (zoom - parentZoom), &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndPoiBox::kLeftFieldNumber, tileX - pTileX, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndPoiBox::kTopFieldNumber, tileY - pTileY, &dataOut);
		stackBounds.push_front(Bounds(tileX, zoom, tileY, 0));

		if (end) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndPoiBox::kShiftToDataFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
			std::shared_ptr<BinaryFileReference> shift(BinaryFileReference::createShiftReference(getFilePointer(), startPoiIndex));
			wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
			return shift;
		}
		return std::shared_ptr<BinaryFileReference>();
	}

	 void BinaryMapDataWriter::endWritePoiBox(){
		popState(POI_BOX);
		writeInt32Size();
		stackBounds.pop_front();
	}


	 void BinaryMapDataWriter::startWriteRouteIndex(std::string name) {
		pushState(ROUTE_INDEX_INIT, OSMAND_STRUCTURE_INIT);
		wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kRoutingIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		preserveInt32Size();
		
		if (name != "") {
			wfl::WireFormatLite::WriteString(obf::OsmAndRoutingIndex::kNameFieldNumber, name, &dataOut);
		}
	}
	
	
	void BinaryMapDataWriter::endWriteRouteIndex(){
		popState(ROUTE_INDEX_INIT);
		int len = writeInt32Size();
	}

	void BinaryMapDataWriter::writeRouteEncodingRules(std::list<MapRouteType> types)  {
		int peeker[] = {ROUTE_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));

		std::vector<MapRouteType> out;
		out.insert(out.end(), types.begin(), types.end());
		// 2. sort by frequency and assign ids
		std::sort(out.begin(), out.end());

		for (int i = 0; i < out.size(); i++) {
			obf::OsmAndRoutingIndex_RouteEncodingRule builder;
			MapRouteType rule = out[i];
			rule.setTargetId(i + 1);

			builder.set_tag(rule.getTag());
			if (rule.getValue() != "") {
				builder.set_value(rule.getValue());
			} else {
				builder.set_value("");
			}
			
			wfl::WireFormatLite::WriteMessage(obf::OsmAndRoutingIndex::kRulesFieldNumber, builder, &dataOut);
		}
	}

	std::unique_ptr<BinaryFileReference> BinaryMapDataWriter::startRouteTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, bool basemap) 
	{
		int peeker[] = {ROUTE_TREE, ROUTE_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		if (states.front() == ROUTE_INDEX_INIT) {
			if(basemap) {
				wfl::WireFormatLite::WriteTag(obf::OsmAndRoutingIndex::kBasemapBoxesFieldNumber,  wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
			} else {
				wfl::WireFormatLite::WriteTag(obf::OsmAndRoutingIndex::kRootBoxesFieldNumber,  wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
			}
		} else {
			wfl::WireFormatLite::WriteTag(obf::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber,  wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		}
		pushState(ROUTE_TREE);
		preserveInt32Size();
		__int64 fp = getFilePointer();

		
		
		Bounds bounds(0, 0, 0, 0); 
		if(!stackBounds.empty()) {
			bounds = stackBounds.front(); 
		}
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber, leftX - bounds.left, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber, rightX - bounds.right, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber, topY - bounds.top, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber, bottomY - bounds.bottom, &dataOut);
		stackBounds.push_front(Bounds(leftX, rightX, topY, bottomY));
		std::unique_ptr<BinaryFileReference> ref;
		if (containsObjects) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
			ref = std::unique_ptr<BinaryFileReference>(BinaryFileReference::createShiftReference(getFilePointer(), fp));
			wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
		}
		return ref;
	}

	void BinaryMapDataWriter::endRouteTreeElement(){
		popState(ROUTE_TREE);
		stackBounds.pop_front();
		writeInt32Size();
	}

	obf::RouteData BinaryMapDataWriter::writeRouteData(int diffId, int pleft, int ptop, std::vector<int> types, std::vector<std::tuple<int, int, int, std::vector<int>>>  points, 
		std::unordered_map<MapRouteType, std::string, hashMapRoute, equalMapRoute>& names, std::unordered_map<std::string, int>& stringTable, obf::OsmAndRoutingIndex_RouteDataBlock& dataBlock,
			bool allowCoordinateSimplification, bool writePointId)
			 {
		obf::RouteData builder;
		builder.set_routeid(diffId);

		// types
		mapDataBuf.clear();
		for (int i = 0; i < types.size(); i++) {
			writeRawVarint32(mapDataBuf, types[i]);
		}
		builder.set_types(mapDataBuf.data(), mapDataBuf.size());
		// coordinates and point types
		int pcalcx = pleft >> ROUTE_SHIFT_COORDINATES;
		int pcalcy = ptop >> ROUTE_SHIFT_COORDINATES;
		mapDataBuf.clear();
		typesDataBuf.clear();
		for(int k=0; k<points.size(); k++) {
			//ROUTE_COORDINATES_COUNT++;
			
			int tx = (std::get<1>(points[k]) >> ROUTE_SHIFT_COORDINATES) - pcalcx;
			int ty = (std::get<2>(points[k]) >> ROUTE_SHIFT_COORDINATES) - pcalcy;
			writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(tx));
			writeRawVarint32(mapDataBuf, wfl::WireFormatLite::ZigZagEncode32(ty));
			pcalcx = pcalcx + tx ;
			pcalcy = pcalcy + ty ;
			if(std::get<3>(points[k]).size() >0) {
				typesAddDataBuf.clear();
				for(int ij =0; ij < std::get<3>(points[k]).size(); ij++){
					writeRawVarint32(typesAddDataBuf, std::get<3>(points[k])[ij]);
				}
				writeRawVarint32(typesDataBuf, k);
				writeRawVarint32(typesDataBuf, typesAddDataBuf.size());
				typesDataBuf.insert(typesDataBuf.end(), typesAddDataBuf.begin(), typesAddDataBuf.end());
			}
		}
		builder.set_points(mapDataBuf.data(), mapDataBuf.size());

		builder.set_pointtypes(typesDataBuf.data(), typesDataBuf.size());

		if (names.size() > 0) {
			mapDataBuf.clear();
			
				for (std::pair<MapRouteType, std::string> s : names) {
					writeRawVarint32(mapDataBuf, s.first.getTargetId());
					int ls = 0;
					if (stringTable.find(s.second) == stringTable.end()) {
						ls = stringTable.size();
						stringTable.insert(std::make_pair(s.second, ls));
					}
					else
					{
						ls = stringTable[s.second];
					}
					writeRawVarint32(mapDataBuf, ls);
				}

				builder.set_stringnames(mapDataBuf.data(), mapDataBuf.size());
		}
		
		return builder;
	}

	void BinaryMapDataWriter::writeRouteDataBlock(obf::OsmAndRoutingIndex_RouteDataBlock& builder, std::unordered_map<std::string, int>& stringTable , BinaryFileReference& ref)
	{
		int peeker[] = {ROUTE_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		
		obf::StringTable bs;
		if (!stringTable.empty()) {
			std::list<std::pair<std::string, int>> sortList;
			for (std::pair<std::string,int> s : stringTable) {
				sortList.push_back(s);
			}

			sortList.sort([](std::pair<std::string, int> elemSort1, std::pair<std::string, int> elem2)
			{
				return elemSort1.second < elem2.second;
			});

			for (std::pair<std::string,int> s : sortList) {
				bs.add_s(s.first);
			}
			builder.mutable_stringtable()->MergeFrom(bs);
		}
		

		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber,  wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE),&dataOut);
		
		ref.writeReference(*raf, getFilePointer());
		dataOut.WriteVarint32(builder.ByteSize());
		builder.SerializeToCodedStream(&dataOut);

	}

	void BinaryMapDataWriter::close()
	{
		int peeker[] = {OSMAND_STRUCTURE_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		popState(OSMAND_STRUCTURE_INIT);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndStructure::kVersionConfirmFieldNumber, 2, &dataOut);

		int buffercount = dataOut.ByteCount();
		int streamCount = raf->ByteCount();
	}


