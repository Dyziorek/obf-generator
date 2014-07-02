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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

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
	google::protobuf::uint32 result = wfl::WireFormatLite::ZigZagEncode32(toVarint32);
	while (result & 0x7F != 0)
	{
		mapDataBuf.push_back((result & 0x7F) | 0x80);
		result >>= 7;
	}
	mapDataBuf.push_back(result);
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
			std::vector<int> addtypeUse, std::map<MapRulType, std::string>& names, boost::unordered_map<std::string, int>& stringTable, obf::MapDataBlock* dataBlock,
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
			writeRawVarint32(mapDataBuf, tx);
			writeRawVarint32(mapDataBuf, ty);
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

					writeRawVarint32(mapDataBuf, tx);
					writeRawVarint32(mapDataBuf, ty);

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
					int ls = stringTable.size();
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

	void BinaryMapDataWriter::writeMapDataBlock(obf::MapDataBlock* builder, boost::unordered_map<std::string, int>& stringTable, BinaryFileReference& ref)
	 {
		 int vacP[] = {MAP_ROOT_LEVEL_INIT};
		checkPeek(vacP, sizeof(vacP)/sizeof(int));
		obf::StringTable bs;
		if (!stringTable.empty()) {
			for (std::pair<std::string,int> s : stringTable) {
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

	boost::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> BinaryMapDataWriter::writeIndexedTable(int tag, std::list<std::string> indexedTable)  {
		wfl::WireFormatLite::WriteTag(tag, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();
		boost::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> res;
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

	void BinaryMapDataWriter::writeAddressNameIndex(boost::unordered_map<std::string, std::list<std::shared_ptr<MapObject>>> namesIndex){
		int peeker[] = {ADDRESS_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		wfl::WireFormatLite::WriteTag(obf::OsmAndAddressIndex::kNameIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();
		 std::list<std::string> namesIndexKeys;
		 for (auto pairData : namesIndex)
		 {
			 namesIndexKeys.push_back(pairData.first);
		 }
		
		 boost::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> res = writeIndexedTable(obf::OsmAndAddressNameIndexData::kTableFieldNumber, namesIndexKeys);
		for(auto entry : namesIndex) {
			std::shared_ptr<BinaryFileReference> ref = res[entry.first];
			wfl::WireFormatLite::WriteTag(obf::OsmAndAddressNameIndexData::kAtomFieldNumber, wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE), &dataOut);
			long pointer = getFilePointer();
			if(ref != nullptr) {
				ref->writeReference(*raf, getFilePointer());
			}
			obf::OsmAndAddressNameIndexData builder;
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
				atom.add_shifttocityindex((int) (pointer - oc->getFileOffset()));
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
			cityInd.set_name_en(city.getEnName());
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
			boost::unordered_map<__int64,std::set<Street>>& mapNodeToStreet, boost::unordered_map<Street, std::list<EntityNode>>& wayNodes){
		int peeker[] = {CITY_INDEX_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		 obf::StreetIndex streetBuilder;
		streetBuilder.set_name(street.getName());
		if (checkEnNameToWrite(street)) {
			streetBuilder.set_name_en(street.getEnName());
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
				bbuilder.set_name_en(b.getEnName());
			}
			if (postcodeFilter == "" && b.postCode != "") {
				bbuilder.set_postcode(b.postCode);
			}
			streetBuilder.add_buildings()->MergeFrom(bbuilder);
		}

		if(!wayNodes.empty()) {
			boost::unordered_set<Street> checkedStreets ;
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
						builder.set_name_en(streetJ.getEnName());
					}
					streetBuilder.add_intersections()->MergeFrom(builder);
				}
			}
		}

		return streetBuilder;
	}


	void BinaryMapDataWriter::writeCityIndex(CityObj cityOrPostcode, std::list<Street>& streets, boost::unordered_map<Street, std::list<EntityNode>>& wayNodes, 
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
		boost::unordered_map<__int64,std::set<Street>> mapNodeToStreet;
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
			currentPointer +=  wfl::WireFormatLite::MessageSize(streetInd);
			cityInd.add_streets()->MergeFrom(streetInd);
			
		}
		obf::CityBlockIndex block;
		int size = wfl::WireFormatLite::UInt32Size(wfl::WireFormatLite::MessageSize(block));
		block.SerializeToCodedStream(&dataOut);
		for (Street s : streets) {
			s.setFileOffset(s.getFileOffset() + size);
		}
	}


	void BinaryMapDataWriter::close()
	{
		int peeker[] = {OSMAND_STRUCTURE_INIT};
		checkPeek(peeker, sizeof(peeker)/sizeof(int));
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndStructure::kVersionConfirmFieldNumber, 2, &dataOut);

		int buffercount = dataOut.ByteCount();
		int streamCount = raf->ByteCount();
	}
