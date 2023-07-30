//
//  jpeg.hpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#ifndef jpeg_hpp
#define jpeg_hpp

#include <istream>

#include "huffmantable.hpp"

class Jpeg {
private:
    std::vector<uint8_t> _identifier;
    uint16_t _version;
    uint8_t _units;
    
    typedef std::array<uint8_t, 64> QuantisationTable;
    std::array<QuantisationTable, 4> _quantTables;
    
    std::array<HuffmanTable, 4> _huffmanTablesAC;
    std::array<HuffmanTable, 4> _huffmanTablesDC;
    
    uint8_t _frameSamplePrecision;
    uint16_t _y; //number of lines
    uint16_t _x; //number of samples per line
    
    struct ImageComponent {
        uint8_t _c; //component identifier
        uint8_t _h; //horizontal sampling factor
        uint8_t _v; //vertical sampling factor
        uint8_t _tq; //quantization table destination selector
    };
    std::vector<ImageComponent> _imageComponents;
    
    struct ImageComponentInScan {
        uint8_t _cs; //scan component selector
        uint8_t _td; //dc entropy coding table destination selector
        uint8_t _ta; //ac entropy coding table destination selector
    };
    std::vector<ImageComponentInScan> _imageComponentsInScan;
    
    typedef std::array<int, 8*8> DataUnit;
    typedef std::tuple<int, int, int> Colour;
    
    size_t _numberOfMCU = 0;
    bool _inScan = false;
    
public:
    Jpeg(std::istream& is);
    
    void readData(std::istream& is);
    void readScanData(std::istream& is);
    DataUnit readBlock(HuffmanDecoder& dec, ImageComponentInScan ic, bool resetDC);
    uint8_t deZigZag(uint8_t index);
    DataUnit dequantiseBlock(DataUnit du, ImageComponent ic);
    DataUnit idct(DataUnit du);
    
    void appZeroData(std::vector<uint8_t>& data);
    void quantisationTable(std::vector<uint8_t>& data);
    void huffmanTable(std::vector<uint8_t>& data);

    void sofBaselineDCT(std::vector<uint8_t>& data);
    void startOfScan(std::vector<uint8_t>& data);
    void restartInterval(std::vector<uint8_t>& data);
    
};

#endif /* jpeg_hpp */
