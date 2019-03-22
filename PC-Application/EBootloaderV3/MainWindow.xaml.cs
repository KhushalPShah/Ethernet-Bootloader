using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;

namespace EBootloaderV3
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 
    public partial class MainWindow : Window
    {

        private TcpClient tcpMain;
        NetworkStream streamMain;
        private StreamWriter SwSender;
        private StreamReader SrReciever;
        private Thread thrMessaging;
        private delegate void UpdateLogCallBack(string strMessage);
       


        public MainWindow()
        {
            InitializeComponent();
        }

        private void btnConnect_Click(object sender, RoutedEventArgs e)
        {
            if((txtBxPort.Text.ToString()=="") || (txtBxIP1.Text.ToString()=="") || (txtBxIP2.Text.ToString() == "") || (txtBxIP3.Text.ToString() == "") || (txtBxIP4.Text.ToString() == ""))
            {
                MessageBox.Show("Please enter all the required details first!", "Error", MessageBoxButton.OK);
                return;
            }
            tcpMain = new TcpClient();
            ///lstBxStatus.Items.Add("connecting...");
            int portNo=Convert.ToInt32(txtBxPort.Text);
            
            String IPAddr;
            IPAddr =txtBxIP1.Text.ToString()+"."+txtBxIP2.Text.ToString()+"."+txtBxIP3.Text.ToString()+"."+txtBxIP4.Text.ToString();
            
            tcpMain.SendBufferSize = 1500;
            tcpMain.Connect(IPAddress.Parse(IPAddr),portNo);
            //lstBxStatus.Items.Add("Connected to "+IPAddr);
           

            
            
        }

        private void btnDisconnect_Click(object sender, RoutedEventArgs e)
        {
            if (tcpMain != null)
            {
                //Reffered from a StackOverflow Site.
                //https://stackoverflow.com/questions/425235/how-to-properly-and-completely-close-reset-a-tcpclient-connection

                tcpMain.GetStream().Close();        
                tcpMain.Close();

            }
            else
                MessageBox.Show("Not Connected to any Server!", "Error", MessageBoxButton.OK);

        }

        public void btnSend_Click(object sender, RoutedEventArgs e)
        {
            //byte[] msg = Encoding.ASCII.GetBytes(txtBxMsg.Text);
            
            NetworkStream stream = tcpMain.GetStream();
           // stream.Write(msg, 0, msg.Length);
            //lstBxStatus.Items.Add("You(Client) sent : "+txtBxMsg.Text.ToString());
            //txtBxMsg.Clear();
        }

        private void btnRecvMsg_Click(object sender, RoutedEventArgs e)
        {
            Byte[] data = new Byte[256];
            String recvMsg = String.Empty;
            NetworkStream stream = tcpMain.GetStream();
            Int32 byteCount = stream.Read(data, 0, data.Length);
            recvMsg = System.Text.Encoding.ASCII.GetString(data, 0,byteCount);
           // txtBxRecvMsg.Text = recvMsg.ToString();
        }

        private void btnFile_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog fileObj = new Microsoft.Win32.OpenFileDialog();
            fileObj.DefaultExt = ".txt";
            fileObj.Filter = "HEX Files (.hex)|*.hex";
            Nullable<bool> result = fileObj.ShowDialog();
            if (result == true)
            {
                string fileName = fileObj.FileName;
                txtBxFileLocation.Text = fileName;
            }

        }
        private void btnProgram_Click(object sender, RoutedEventArgs e)
        {
            //Decoding the File:
            FileStream fs = new FileStream(txtBxFileLocation.Text, FileMode.Open);
            FileInfo fi = new FileInfo(txtBxFileLocation.Text);
            byte[] MMArray = new byte[458752];       //448kB (512-64kB(Bootloader))
            int colonCheck = 0;             //check for the beginning of HEX line with a colon.
            int bytCntTens = 0, bytCntUnits = 0, bytCnt = 0;
            int addrTens = 0, addrUnits = 0, addrUpHalf = 0, addrLwHalf = 0, addr = 0;
            int rcdTypTens = 0, rcdTypUnits = 0, rcdTyp = 0;
            int extAddr = 0;
            int[] extAddrArray = new int[8];    //only of 8 as the record type's address will never exceed beyond 8. 512K=80000H
            int dataTens = 0, dataUnits = 0, data = 0;
            int addrMax = 0;


            for (int i = 0; i < MMArray.Length; i++)
                MMArray[i] = 0xFF;

            for (int i = 0; i < extAddrArray.Length; i++)
                extAddrArray[i] = 0;

            //fs.Read will read the ASCII values.
            for (int i = 0; i < 516095; i++)
            {
                colonCheck = fs.ReadByte();
                if (colonCheck == 58)      //the ASCII value of COLON.
                {
                    //for Byte-Count:
                    bytCntTens = fs.ReadByte();     //read the first digit 
                    bytCntUnits = fs.ReadByte();    // read the second digit 
                    bytCnt = Merge(bytCntTens, bytCntUnits);    //merge the two and get the Byte-Count in decimal format.

                    //for Address:
                    addrTens = fs.ReadByte();
                    addrUnits = fs.ReadByte();
                    addrUpHalf = Merge(addrTens, addrUnits);    //decimal value for first 2 bytes of address.

                    addrTens = fs.ReadByte();
                    addrUnits = fs.ReadByte();
                    addrLwHalf = Merge(addrTens, addrUnits);    //decimal value for last 2 bytes of address.

                    //for Record Type:
                    rcdTypTens = fs.ReadByte();
                    rcdTypUnits = fs.ReadByte();
                    rcdTyp = Merge(rcdTypTens, rcdTypUnits);    //decimal value for record type.

                    addr = extAddrArray[extAddr] * 16;
                    addr += ((addrUpHalf * 256) + addrLwHalf);


                    if (rcdTyp == 0 || rcdTyp == 3)    //The data of Record Type 3 is avoided for the time being.Check INFO1 in Req.of ARM side Code.
                    {
                        for (int x = 0; x < bytCnt; x++)
                        {
                            dataTens = fs.ReadByte();
                            dataUnits = fs.ReadByte();
                            data = Merge(dataTens, dataUnits);
                            MMArray[addr] = (byte)data;         //store the data in the specified location in the M/M array in Byte form.
                            addr++;                             //increment the Address pointer.
                        }
                    }

                    else if (rcdTyp == 1)
                    {
                        //Indicates Enf Of File.
                    }


                    else if (rcdTyp == 2)
                    {
                        extAddr++;

                        addrTens = fs.ReadByte();
                        addrUnits = fs.ReadByte();
                        addrUpHalf = Merge(addrTens, addrUnits);

                        addrTens = fs.ReadByte();
                        addrUnits = fs.ReadByte();
                        addrLwHalf = Merge(addrTens, addrUnits);

                        extAddrArray[extAddr] = ((addrUpHalf * 256) + addrLwHalf);
                    }
                    if (addr > addrMax)
                        addrMax = addr;


                    //for Check sum
                    fs.ReadByte();
                    fs.ReadByte();

                    // for another line. This is not visible in the text format but at the back End it is there.
                    fs.ReadByte();          //for /r    
                    fs.ReadByte();          //for /n

                }


            }
            fs.Close();
            //File decoding Done!

            //Auto Detect Device has to be Done here!
            /*
             * Things to be Done:
             * Connect to the Controller.
             * Send Data to the Controller 
             * Wait for the same Data to be reverted back by the Controller 
             * If that succeeds, assure device detected, else Not.
             * 
             **/
            /*tcpMain = new TcpClient();
            int portNo = Convert.ToInt32(txtBxPort.Text);

            String IPAddr;
            IPAddr = txtBxIP1.Text.ToString() + "." + txtBxIP2.Text.ToString() + "." + txtBxIP3.Text.ToString() + "." + txtBxIP4.Text.ToString();

            tcpMain.SendBufferSize = 1500;
            tcpMain.Connect(IPAddress.Parse(IPAddr), portNo);

            streamMain = tcpMain.GetStream();
            **/
            //Comment Recent.
            DetectDevice detDev = new DetectDevice(tcpMain, streamMain, txtBxIP1.Text.ToString(), txtBxIP2.Text.ToString(), txtBxIP3.Text.ToString(), txtBxIP4.Text.ToString(), Convert.ToInt32(txtBxPort.Text));
            detDev.ShowDialog();
            if (detDev.isNotDetected) return;
            streamMain = detDev.stream;
            tcpMain = detDev.tcp;
            
            pgrsBarUpload.Maximum = addrMax;
            pgrsBarUpload.Value = 0;

            int sectorsSent = 0;
            int segmentSize = 512;
            int segmentSent = 0;
            int addrBeginPage = 65536;   
            int pageMaxAddr = 4096;
            int tempAddr = 0;
            byte[] addrArray = new byte[2];
            byte[] tempDataSend = new byte[pageMaxAddr];
            byte[] tempDataRecv = new byte[pageMaxAddr+8];
            int error = 0;
            int errorCnt = 0;
            int addrFull = 65536;
            int extAddrIndex = 0;
            byte[] TxExt = new byte[2];
            byte[] finalAddrPacket = new byte[4];
            byte[] recvAddr = new byte[4];
           
            Int32 recvAddrCount=0;
            Int32 recvDataCount = 0;

            
            Thread t = new Thread(() =>
            {
                try
                {   
                    while (addrBeginPage < addrMax)
                    {
                        extAddrIndex = (addrBeginPage / addrFull);
                        if (addrBeginPage < addrFull)
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);

                        }
                        else if (addrBeginPage < (addrFull * 2))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 3))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 4))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 5))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 6))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 7))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }
                        else if (addrBeginPage < (addrFull * 8))
                        {
                            finalAddrPacket[0] = (byte)(extAddrArray[extAddrIndex] >> 8);
                            finalAddrPacket[1] = (byte)(extAddrArray[extAddrIndex] & 0xFF);
                        }

                        tempAddr = addrBeginPage;                               //BeginPage is 8192 i.e. 8Kb.
                        finalAddrPacket[2] = (byte)(addrBeginPage >> 8);
                        finalAddrPacket[3] = (byte)(addrBeginPage & 0xFF);

                        

                        //Comment Recent.
                        streamMain.Write(finalAddrPacket, 0, 4);
                
                        while(recvAddrCount<4)
                        {
                            recvAddrCount = streamMain.Read(tempDataRecv, 0, 4);
                            recvAddrCount += recvAddrCount;
                        }
                        recvAddrCount = 0;
                        while(segmentSent<8)
                        {
                            for (int i = 0; i < segmentSize; i++)
                            {
                                tempDataSend[i] = MMArray[addrBeginPage];             //Data at the addressed Location.
                                addrBeginPage++;
                            }
                            streamMain.Write(tempDataSend, 0, 512);
                            segmentSent++;
                            
                            if (segmentSent==8)
                            {
                                byte[] buffer = new byte[9];
                                int bytesRead = 0;
                                int totalBytesRead = 0;
                                while (totalBytesRead < 9)
                                {
                                    bytesRead = streamMain.Read(tempDataRecv, 0, 9);
                                    totalBytesRead += bytesRead; 
                                }
                            }
                            else
                            {
                                byte[] buffer = new byte[1]; 
                                int bytesRead=0;
                                int totalBytesRead=0;
                                 while(totalBytesRead < 1)
                                {
                                    //bytesRead = streamMain.Read(buffer, 0, 512);
                                    //totalBytesRead += bytesRead;
                                    bytesRead = streamMain.Read(buffer, 0, 1);
                                    totalBytesRead += bytesRead;

                                }

                            }
                            {
                                pgrsBarUpload.Dispatcher.Invoke(() =>
                                {
                                    pgrsBarUpload.Value = (addrBeginPage);
                                });

                            }

                            recvDataCount = 0;

                        }
                        sectorsSent++;
                        segmentSent = 0;
                
                        if (errorCnt == 2)
                        {
                            errorCnt = 0;
                            MessageBox.Show("Error in Programming", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                            return;
                        }
                        

                    }
            

            }
            catch (Exception ex)
            {
                    MessageBox.Show("No Device Connected." + ex.Message + "Please Select appropiate COM PORT", "DEVICE NOT FOUND", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;

            }
            MessageBox.Show("Done Flashing the Program", "COMPLETED", MessageBoxButton.OK, MessageBoxImage.Information);
            tcpMain.GetStream().Close();
            tcpMain.Close();


            });
            t.Start();
        }

        
    
        int Merge(int a, int b)
        {
                int merged;
                a = ASCToDec(a);
                b = ASCToDec(b);
                merged = ((a * 16) + b);

                return merged;
        }
        int ASCToDec(int a)
        {
                if (a < 58)
                    a = a - 48;         // if from 0 to 9.
                else
                    a = a - 55;         // if from A to F.

                return a;
        }

    }
}


