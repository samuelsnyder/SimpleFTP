/*
 * ftclient.java
 * Sam Snyder
 * samuel.leason.snyder@gmail.com
 * Reference: 
 *	Beej's guide: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * 	Java Networking tutorial: https://docs.oracle.com/javase/tutorial/networking/sockets/
 */

import java.io.*;
import java.net.*;

public class FtClient {

	public static void main(String[] args) throws IOException{

		// validate input: java FtCLient SERVER_HOST SERVER_PORT COMMAND FILENAME
		// java FtClient <SERVER_HOST> <SERVER_PORT> <COMMAND> <DATA_PORT> [<FILENAME>] 
		if(args.length != 4 && args.length != 5){
			System.out.println("Usage: java FtClient <SERVER_HOST> <SERVER_PORT> <COMMAND> <DATA_PORT> [<FILENAME>]");
			System.exit(1);
		}

		// input parameters, initial values provided to placate compiler
		String server_host = args[0];
		int server_port = Integer.parseInt(args[1]);
		String command = args[2]; 
		int data_port = Integer.parseInt(args[3]);
		String filename = "";
		if(args.length == 5){
			filename = args[4];
		}

		// if get, must give file name
		if ( command.equals("-g") && args.length == 4) {
			System.out.println("<FILENAME> must be specified for command '-g'");
			System.exit(1);
		}

		if (command.equals("-g") == true){
			// check if file already exists
			boolean fileExistsLocally = new File(filename).exists();
			if (fileExistsLocally == true)
			{
				System.out.println("Local directory already contains file named " +  filename);
				System.exit(1);
			}
		}

		try{

			// create buffered reader object to read keyboard input
			BufferedReader inputReader = new BufferedReader( new InputStreamReader(System.in));

			// make control socket
			Socket controlSocket = new Socket(server_host, server_port);
			System.out.println("Connected to ftserver at " + controlSocket.getInetAddress() );

			// make buffered readers and writers for sockets
			// make PrintWriter for writing to control socket
			PrintWriter controlSocketWriter = new PrintWriter(controlSocket.getOutputStream(), true);

			// make BufferedReader for inputStream from sockets
			BufferedReader controlSocketReader = new BufferedReader (new InputStreamReader(controlSocket.getInputStream()));

			// make data server socket
			ServerSocket dataServerSocket = new ServerSocket(data_port);
			System.out.println("waiting for data connection on port " + data_port);

			// send command message to server
			System.out.println("Command being sent to server: " + command + " " + data_port + " " +filename );
			controlSocketWriter.println(command + " " + data_port + " " + filename);

			// create socket, stream reader, buffered reader
			Socket dataSocket = dataServerSocket.accept();
			System.out.println("Accepted new connection on port " + data_port);
			InputStream dataSocketStream = dataSocket.getInputStream();
			BufferedReader dataSocketBufferedReader = new BufferedReader ( new InputStreamReader (dataSocketStream));

			// wait for data socket to send us some data, or for error message to come from control socket
			while (! dataSocketBufferedReader.ready())
			{
				// check if control socket has an error message for us
				if (controlSocketReader.ready()) {
					System.out.println(controlSocketReader.readLine() );
					System.exit(0);
				}
				// no error message yet, or data. Check again.
				continue;
			}

			if (command.equals("-l") ) {
				// this was a list request
				String s;
				// here we read using a buffered reader, and output to the screen.
				while ( (s = dataSocketBufferedReader.readLine()) != null )
					System.out.println(s );

				controlSocket.close();

			}

			if (command.equals("-g")){

				// this was a file request
				System.out.println("Receiving file: " +  filename);
				FileOutputStream fileOutputStream = new FileOutputStream(filename);
				byte[] buffer = new byte[1024];
				int count= 0;

				// here we use a stream rather than a buffered reader to read bytes, not characters
				// and then write them into a file
				while ( (count = dataSocketStream.read(buffer,0,buffer.length) ) > 0 ){
					fileOutputStream.write(buffer, 0, count);
				}
				// All done. Close command connection
				controlSocket.close();
				System.out.println("Done receiving file: " +  filename);
			}
		}
		// catch UnknownHostException
		catch(UnknownHostException e){
			System.err.println("UnknownHostException: " + e.getMessage());
		}
		// catch IOException
		catch(IOException e){
			System.err.println("IOException: " + e.getMessage());
		}
	}
}
