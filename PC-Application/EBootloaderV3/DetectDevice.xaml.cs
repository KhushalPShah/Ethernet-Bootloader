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
using System.Windows.Shapes;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;

namespace EBootloaderV3
{
    /// <summary>
    /// Interaction logic for DetectDevice.xaml
    /// </summary>
    public partial class DetectDevice : Window
    {
        public NetworkStream stream;
        public TcpClient tcp;

        public bool isNotDetected = true;

        int portNumber;
        string IP1;
        string IP2;
        string IP3;
        string IP4;

        

        public  DetectDevice(TcpClient tcpMain,NetworkStream streamMain,string IP1,string IP2,string IP3,string IP4,int portNumber)
        {
            InitializeComponent();

            //tcp = new TcpClient();

            this.tcp = tcpMain;
            this.stream = streamMain;
            this.IP1 = IP1;
            this.IP2 = IP2;
            this.IP3 = IP3;
            this.IP4 = IP4;
            this.portNumber = portNumber;

        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            Thread t = new Thread(() =>
            {

                try
                {
                    tcp = new TcpClient();
                    string IPAddr;
                    IPAddr = IP1 + "." + IP2 + "." + IP3 + "." + IP4;

                    //tcp.SendBufferSize = 1500;
                        tcp.Connect(IPAddress.Parse(IPAddr), portNumber);
                   
                    stream = tcp.GetStream();
                    byte[] ack = new byte[1];
                    ack[0] = (byte)3;
                    stream.Write(ack, 0, 1);

                    byte[] ackRecv = new Byte[1];
                    Int32 byteCount = stream.Read(ackRecv, 0, ackRecv.Length);
                    if(byteCount==1 && ackRecv[0]==(byte)3)
                    {
                        isNotDetected = false;
                        this.Dispatcher.Invoke(() =>
                        {
                            this.Close();
                        });

                    }

                }
                catch(TimeoutException ex)
                {
                    MessageBox.Show("Read timeout. Please try again", "Detect Device", MessageBoxButton.OK, MessageBoxImage.Error);
                    this.Dispatcher.Invoke(() =>
                    {
                        this.Close();
                    });
                }
                catch(Exception e1)
                {
                    MessageBox.Show("Error connecting"+ e1);
                }
            });
            t.Start();
        }
    }
}
