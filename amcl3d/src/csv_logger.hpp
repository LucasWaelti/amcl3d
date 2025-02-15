// Description: A simple CSV logger for logging data to CSV files.
// Author:  Lucas Waelti (GitHub: LucasWaelti)
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdarg>
#include <unordered_map>

#include <Eigen/Dense>

/**
 * @brief   The CSV logger is a simple logger for logging data to several CSV files.
 * @param   data_directory The directory where the csv files are saved. The default directory is /tmp/.
 * @param   separator The separator between the values in the CSV file. The default separator is a comma ','.
 * @warning It is not advised to use the space ' ' as a separator.
 * @details     This file contains a simple CSV logger for logging data to CSV
 *              files. The logger can be used to log data to multiple CSV files
 *              simultaneously. The logger can be used as follows:
 *              1. Initialize a CSV file using the init() function. This
 *                  function takes two arguments: the filename and the header. The
 *                  filename is the name of the CSV file (including the .csv
 *                  extension). The header is a string containing the header of the
 *                  CSV file. The header must use the specified separator. 
 *              2. Log data to the CSV file using the log() function. This function
 *                  takes a variable number of arguments. The first argument is the
 *                  filename. The remaining arguments are the values to log. The number of
 *                  values should match the number of columns in the CSV file. The function
 *                  returns true if the data was successfully logged, false otherwise.
 *              3. Close the CSV file using the close(filename) function. This function takes
 *                  one argument: the filename. The function returns true if the file was
 *                  successfully closed, false otherwise.
 *              4. Close all CSV files using the close() function without arguments.
*/
class CSVLogger{

public:

    // Logger parameters 
    std::string data_directory   = "/tmp/";  // The data is saved in the /tmp/ directory by default
    std::string separator        = ",";     // The separator between the values in the CSV file

    // Store the file streams in a map 
    std::unordered_map<std::string, std::ofstream*>     file_streams;
    std::unordered_map<std::string, std::stringstream*> file_buffers;

    /**
     * @brief       Set the directory where the data is saved. 
     *              Adds a trailing slash if not present.
     * @param[in]   dir The directory
     * @note        The default directory is /tmp/
    */
    void set_data_dir(std::string dir){

        data_directory = dir;
        
        if(data_directory.back() != '/')
            data_directory += "/";
    }

    /**
     * @brief       Set the separator between the values in the CSV file
     * @param[in]   sep The separator
     * @note        The default separator is a comma
     */
    void set_separator(const char& sep = ','){
        separator = sep;
    }

    /**
     * @brief       Remove the trailing separator from a string
     * @param[in]   line The string
     * @warning     The exact separator must be at the end of the string
     */
    void remove_trailing_separator(std::string& line){
        if(line.back() == separator[0])
            line.pop_back();
    }

    /**
     * @brief      Remove spaces from a string
     * @param[out] str The string
     * @note       If the separator is a space, the string is returned as is
     */
    void remove_spaces(std::string& str){
        if(separator == " ") return; // Do not remove spaces if the separator is a space
        str.erase(remove(str.begin(), str.end(), ' '), str.end());
    }

    /**
     * @brief       Generate a CSV header from the dimensions and names of the matrices.
     *              Note: the matrices are assumed to be in RowMajor
     *              format. All elements are named <name>RC where R is
     *              the row index and C is the column index.
     * @param[in]   dims The dimensions of the matrices (vector of {rows, cols} pairs)
     * @param[in]   names The name of the matrices
     * @return      std::string: the header string
    */
    std::string matrix_header(std::vector<std::pair<int,int>> dims, std::vector<std::string> names){

        // Check the number of matrices and names match
        if(dims.size() != names.size()){
            std::cerr << "Error: number of matrices and names do not match" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Initialize the header string
        std::string header = "";

        // Loop through the matrices and add the column names to the header string
        for(int i=0; i<dims.size(); i++){
            for(int r=0; r<dims[i].first; r++){
                for(int c=0; c<dims[i].second; c++){
                    header += names[i] + std::to_string(r) + std::to_string(c) + separator;
                }
            }
        }

        remove_trailing_separator(header);

        return header; 
    }

    /**
     * @brief       Check if a file is already opened
     * @param[in]   filename The file name
     * @return      bool: true if the file is already opened, false otherwise
     */
    bool is_file_opened(std::string filename){
        return file_streams.find(filename) != file_streams.end() && file_streams[filename]->is_open();
    }


    /**
     * @brief      Initializes a new CSV file (either creates or overwrites)
     * @param[in]  filename The file name (no path, with the .csv extension)
     * @param[in]  header   The header (comma separated)
     * @return     -1 if the file could not be opened, 0 if the file was opened successfully, 1 if the file was already opened
    */
    int init(std::string filename, std::string header = ""){

        if(is_file_opened(filename))
            return 1;

        // Create the directory if it does not exist yet 
        std::string command = "mkdir -p " + data_directory;
        if(system(command.c_str()))
            std::cerr << "Error creating directory: " << data_directory << std::endl;

        // Open the CSV file for writing (overwrite if it already exists)
        file_streams[filename] = new std::ofstream(data_directory + filename, std::ios::trunc);

        // Initialize the buffer for the file
        file_buffers[filename] = new std::stringstream();
        
        // Check the file is opened correctly
        if (!file_streams[filename]->is_open()) {
            std::cerr << "Error opening file: " << data_directory << filename << std::endl;
            return -1;
        }
        else
            std::cout << "Opened file: " << data_directory << filename << std::endl;

        // Write the header to the CSV file if provided
        if(header != ""){
            remove_spaces(header);
            remove_trailing_separator(header);
            *file_streams[filename] << header << std::endl;
        }

        // Close the file and reopen it in append mode
        file_streams[filename]->close();
        file_streams[filename]->open(data_directory + filename, std::ios::app);

        return 0;
    }


    /**
     * @brief       Flatten an Eigen::Matrix into a std::vector. Use this
     *              function to flatten an Eigen matrix before logging it 
     *              to a CSV file (RowMajor).
     * @tparam      T The data type of the Eigen matrix (int, float, double, ...)
     * @tparam      Rows The number of rows in the Eigen matrix
     * @tparam      Cols The number of columns in the Eigen matrix
     * @param[in]   matrix The Eigen matrix to flatten 
     * @return      std::vector<T>: the flattened matrix
    */
    template<typename T, int Rows, int Cols>
    std::vector<T> vectorize(const Eigen::Matrix<T,Rows,Cols>& matrix) {
        std::vector<T> vec(matrix.cols()*matrix.rows());
        for(int i=0; i<matrix.rows(); i++){
            for(int j=0; j<matrix.cols(); j++){
                vec[i*matrix.cols() + j] = static_cast<T>(matrix(i,j));
            }
        }
        return vec;
    }


    //
    // Vectors
    //
    template<typename T>
    void write_to_line_(std::stringstream& line, const std::vector<T>& vec) {    
        for(T value: vec)
            line << value << separator;
    }

    //
    // Basic types
    //
    void write_to_line_(std::stringstream& line, const int& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const size_t& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const int64_t& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const float& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const double& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const std::string& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const char* t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const char& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line, const bool& t) {
        line << t << separator;
    }
    void write_to_line_(std::stringstream& line) {
        // Reached the end of the recursion 
    }

    template<typename T, typename... Args>
    void write_to_line_(std::stringstream& line, const T& t, const Args... args) {
        write_to_line_(line, t);         // Write the next argument
        write_to_line_(line, args...);   // Recurse
    }

    /**
     * @brief       Write a new line to a CSV file buffer for an arbitrary number of variables
     *              of arbitrary type. The variables are written to the CSV file in the
     *              order they appear in the argument list.
     * @param[in]   filename The file name
     * @param[in]   args The variables to log (supported types: int, size_t, float, double, std::string or char*, char, bool, std::vector<T>)
     * @return      bool: true if the data was successfully logged.
    */
    template<typename... Args>
    bool log(std::string filename, const Args... args) {

        if(filename == ""){
            throw(std::invalid_argument("Filename cannot be empty"));
        }

        // Check the file is opened correctly
        if(file_buffers.find(filename) == file_buffers.end()){
            std::cerr << "Error, no file buffer: " << filename << std::endl;
            return false;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(9); // Set number of decimals and fixed precision (no exponential notation)
        write_to_line_(ss, args...);

        std::string line = ss.str();
        remove_trailing_separator(line);
        remove_spaces(line);

        // Add stream to file buffer
        *file_buffers[filename] << line << std::endl;
        
        // *file_streams[filename] << line << std::endl;

        return true;
    }

    /**
     * @brief Flush all file stream buffers into their files
    */
    bool flush(){
        for (auto it : file_streams){

            std::string filename = it.first;

            // Check the file is opened correctly
            if(file_streams.find(filename) == file_streams.end() || !file_streams[filename]->is_open()){
                std::cerr << "Error flushing stream to file: " << filename << std::endl;
                return false;
            }

            *it.second << file_buffers[filename]->str();
            file_buffers[filename]->str("");
        }
        return true;
    }

    /**
     * @brief      Close a previously opened CSV file
     * @param[in]  filename The file name
     * @return     bool: true if the file was successfully closed,
    */
    bool close(std::string filename){

        // Check the file was previously opened
        if(file_streams.find(filename) == file_streams.end() || !file_streams[filename]->is_open()){
            std::cerr << "Error closing file: " << filename << std::endl;
            return false;
        }
        else
            std::cout << "Closing file: " << filename << std::endl;

        file_streams[filename]->close();
        delete file_streams[filename];
        file_streams.erase(filename);

        return true;
    }

    /**
     * @brief      Close all previously opened CSV file_streams
    */
    void close(){
        
        for (auto it : file_streams){
            it.second->close();
            delete it.second;
        }
        
        file_streams.clear();

        std::cout << "Closed all file_streams" << std::endl;
    }

};