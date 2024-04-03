using System.Text;
using System.IO;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace KeyEditor
{
	public partial class Form1 : Form
	{
		public List<HellKeyMapping> hellKeyMaps = new List<HellKeyMapping>();
		private int selectedNumber;
		//private int pressedKey;
		private Dictionary<int, string> mapping = new Dictionary<int, string>();
		// Path to the INI file
		string configFilePath = "config.ini";
		bool activated = false;

		public Form1()
		{
			InitializeComponent();
			DisableRightButtons();

			// Assign event handler to all buttons in the right panel
			foreach (Control control in rightPanel.Controls)
			{
				if (control is Button)
				{
					control.Click += rightButton_Click;
				}
			}

			ResetAll();
			lblHelpText.Text = "Press a numpad key button then choose " + Environment.NewLine + "your Stratogem";
			FillMappings();
		}

		private List<string> GetData()
		{
			List<string> dataToWrite = new List<string>();
			if (btnNum0.Image != null)
			{
				dataToWrite.Add(btnNum0.Tag.ToString() + "," + 0);
			}
			if (btnNum1.Image != null)
			{
				dataToWrite.Add(btnNum1.Tag.ToString() + "," + 1);
			}
			if (btnNum2.Image != null)
			{
				dataToWrite.Add(btnNum2.Tag.ToString() + "," + 2);
			}
			if (btnNum3.Image != null)
			{
				dataToWrite.Add(btnNum3.Tag.ToString() + "," + 3);
			}
			if (btnNum4.Image != null)
			{
				dataToWrite.Add(btnNum4.Tag.ToString() + "," + 4);
			}
			if (btnNum5.Image != null)
			{
				dataToWrite.Add(btnNum5.Tag.ToString() + "," + 5);
			}
			if (btnNum6.Image != null)
			{
				dataToWrite.Add(btnNum6.Tag.ToString() + "," + 6);
			}
			if (btnNum7.Image != null)
			{
				dataToWrite.Add(btnNum7.Tag.ToString() + "," + 7);
			}
			if (btnNum8.Image != null)
			{
				dataToWrite.Add(btnNum8.Tag.ToString() + "," + 8);
			}
			if (btnNum9.Image != null)
			{
				dataToWrite.Add(btnNum9.Tag.ToString() + "," + 9);
			}
			return dataToWrite;
		}

		private void DisableRightButtons()
		{
			foreach (Control control in rightPanel.Controls)
			{
				if (control is Button)
				{
					control.Enabled = false;
				}
			}
			btnDeleteHK.Enabled = false;
		}

		private void EnableRightButtons()
		{
			foreach (Control control in rightPanel.Controls)
			{
				if (control is Button)
				{
					control.Enabled = true;
				}
			}
			btnDeleteHK.Enabled = true;
		}

		private void Form1_Load(object sender, EventArgs e)
		{
			LoadSavedIDs();
		}

		private void LoadSavedIDs()
		{
			try
			{
				// Check if the config.ini file exists
				if (File.Exists(configFilePath))
				{
					// Read the content of the config.ini file
					string[] lines = File.ReadAllLines(configFilePath);

					// Search for the line containing saved IDs
					string savedIDsLine = lines.FirstOrDefault(line => line.StartsWith("#SavedIDs"));

					if (savedIDsLine != null)
					{
						// Convert the IDs to integers
						List<int> savedIDs = new List<int>();

						// Extract the IDs from the line
						string[] savedIDsGroupArray = savedIDsLine.Replace("#SavedIDs", "").Split('|');
						foreach (string savedID in savedIDsGroupArray)
						{
							string[] savedIDsArray = savedID.Split(',');

							// Ensure that there are at least two elements in the array
							if (savedIDsArray.Length >= 2)
							{
								// Extract the weapon ID and button ID
								if (int.TryParse(savedIDsArray[0], out int weaponID) && int.TryParse(savedIDsArray[1], out int buttonID))
								{
									savedIDs.Add(weaponID); // Add weapon ID
									savedIDs.Add(buttonID); // Add button ID
									ChangeNumpadButtonColor(buttonID, Color.Green, weaponID.ToString());
								}
							}
						}

					}
				}
				else
				{
					MessageBox.Show("config.ini file not found.");
				}
			}
			catch (Exception ex)
			{
				// Handle any errors that may occur during the file reading process
				MessageBox.Show("Error loading saved IDs: " + ex.Message);
			}
		}

		private void rightButton_Click(object sender, EventArgs e)
		{
			Button button = (Button)sender;
			DisableRightButtons();
			ChangeNumpadButtonColor(selectedNumber, Color.Green, button.Tag.ToString()); // Change background color of the numkey
		}

		private void SetImageButton(Button button, int number)
		{
			try
			{
				string imagePath;
				if (number == 99)
				{
					button.Image = null;
					button.Tag = null;
				}
				else
				{
					// Construct the path to the image file
					imagePath = Path.Combine(Application.StartupPath, "Resources", number + ".jpg");
					// Check if the image file exists
					if (File.Exists(imagePath))
					{
						// Load the image from file
						button.Image = Image.FromFile(imagePath);
						button.Tag = number;
					}
					else
					{
						// Image file does not exist
						MessageBox.Show("Image file not found for number: " + number);
					}
				}

			}
			catch (Exception ex)
			{
				// Exception occurred
				MessageBox.Show("Error: " + ex.Message);
			}
		}

		private void ChangeNumpadButtonColor(int numKey, Color color, string weaponType)
		{
			switch (numKey)
			{
				case 0:
					btnNum0.BackColor = color;
					SetImageButton(btnNum0, int.Parse(weaponType));
					break;
				case 1:
					btnNum1.BackColor = color;
					SetImageButton(btnNum1, int.Parse(weaponType));
					break;
				case 2:
					btnNum2.BackColor = color;
					SetImageButton(btnNum2, int.Parse(weaponType));
					break;
				case 3:
					btnNum3.BackColor = color;
					SetImageButton(btnNum3, int.Parse(weaponType));
					break;
				case 4:
					btnNum4.BackColor = color;
					SetImageButton(btnNum4, int.Parse(weaponType));
					break;
				case 5:
					btnNum5.BackColor = color;
					SetImageButton(btnNum5, int.Parse(weaponType));
					break;
				case 6:
					btnNum6.BackColor = color;
					SetImageButton(btnNum6, int.Parse(weaponType));
					break;
				case 7:
					btnNum7.BackColor = color;
					SetImageButton(btnNum7, int.Parse(weaponType));
					break;
				case 8:
					btnNum8.BackColor = color;
					SetImageButton(btnNum8, int.Parse(weaponType));
					break;
				case 9:
					btnNum9.BackColor = color;
					SetImageButton(btnNum9, int.Parse(weaponType));
					break;
				// Add cases for other num keys as needed
				default:
					// Handle invalid num key
					break;
			}
		}

		private void btnDeleteHK_Click(object sender, EventArgs e)
		{
			ChangeNumpadButtonColor(selectedNumber, Color.LightGray, "99");
		}

		private void btnNum0_Click(object sender, EventArgs e)
		{
			selectedNumber = 0;
			EnableRightButtons();
		}

		private void btnNum1_Click(object sender, EventArgs e)
		{
			selectedNumber = 1;
			EnableRightButtons();
		}

		private void btnNum2_Click(object sender, EventArgs e)
		{
			selectedNumber = 2;
			EnableRightButtons();
		}

		private void btnNum3_Click(object sender, EventArgs e)
		{
			selectedNumber = 3;
			EnableRightButtons();
		}

		private void btnNum4_Click(object sender, EventArgs e)
		{
			selectedNumber = 4;
			EnableRightButtons();
		}

		private void btnNum5_Click(object sender, EventArgs e)
		{
			selectedNumber = 5;
			EnableRightButtons();
		}

		private void btnNum6_Click(object sender, EventArgs e)
		{
			selectedNumber = 6;
			EnableRightButtons();
		}

		private void btnNum7_Click(object sender, EventArgs e)
		{
			selectedNumber = 7;
			EnableRightButtons();
		}

		private void btnNum8_Click(object sender, EventArgs e)
		{
			selectedNumber = 8;
			EnableRightButtons();
		}

		private void btnNum9_Click(object sender, EventArgs e)
		{
			selectedNumber = 9;
			EnableRightButtons();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			ResetAll();
		}

		private void ResetAll()
		{
			btnNum0.BackColor = Color.LightGray;
			btnNum1.BackColor = Color.LightGray;
			btnNum2.BackColor = Color.LightGray;
			btnNum3.BackColor = Color.LightGray;
			btnNum4.BackColor = Color.LightGray;
			btnNum5.BackColor = Color.LightGray;
			btnNum6.BackColor = Color.LightGray;
			btnNum7.BackColor = Color.LightGray;
			btnNum8.BackColor = Color.LightGray;
			btnNum9.BackColor = Color.LightGray;

			btnNum0.Image = null;
			btnNum1.Image = null;
			btnNum2.Image = null;
			btnNum3.Image = null;
			btnNum4.Image = null;
			btnNum5.Image = null;
			btnNum6.Image = null;
			btnNum7.Image = null;
			btnNum8.Image = null;
			btnNum9.Image = null;

			DisableRightButtons();
		}

		private void btnSave_Click(object sender, EventArgs e)
		{
			List<string> dataRecieved = GetData();
			if (dataRecieved != null)
			{
				string combinedString = "# VirtualKeyboard generated config.ini file - Modify if you know what you are doing!\n\nGLOBAL IniVersion hellconfig_v1\nGLOBAL ActiveConfigOnStartup 1\n[CONFIG_1]\nOPTION configName = hellconfig\n\n";
				string saveIDs = "";
				foreach (string data in dataRecieved)
				{
					//Split id and numpad number
					string[] dataArray = data.Split(',');
					HellKeyMapping saveString = GetHellKeyMappingById(dataArray[0]);
					combinedString += saveString.KeyCombinationString + Environment.NewLine;
					saveIDs += saveString.ID.ToString() + "," + dataArray[1] + "|";
				}
				combinedString += $"\n\n#SavedIDs {saveIDs}";
				// Write the string to the file
				File.WriteAllText(configFilePath, combinedString);
			}
		}

		private string Sleep()
		{
			Random random = new Random();
			int randomNumber = random.Next(70, 151); // Generates a random integer between 70 (inclusive) and 150 (exclusive)
			return "SLEEP:" + randomNumber.ToString();
		}

		private void FillMappings()
		{
			//hellKeyMaps.Clear();

			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 0,
				Name = "Mech",
				KeyCombinationString = ParseKeyCombination("UDDRLU") // todo
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 1,
				Name = "APW-1 Anti-Materiel Rifle",
				KeyCombinationString = ParseKeyCombination("DLRUD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 2,
				Name = "MG-206 Heavy Machine Gun",
				KeyCombinationString = ParseKeyCombination("DLUDD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 3,
				Name = "MG-43 Machine Gun",
				KeyCombinationString = ParseKeyCombination("DLDUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 4,
				Name = "M-105 Stalwart",
				KeyCombinationString = ParseKeyCombination("DLDUUL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 5,
				Name = "EAT-17 Expendable Anti-tank",
				KeyCombinationString = ParseKeyCombination("DDLUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 6,
				Name = "GR-8 Recoilless Rifle",
				KeyCombinationString = ParseKeyCombination("DLRRL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 7,
				Name = "FLAM-40 Flamethrower",
				KeyCombinationString = ParseKeyCombination("DLUDU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 8,
				Name = "AC-8 Autocannon",
				KeyCombinationString = ParseKeyCombination("DLDUUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 9,
				Name = "RS-422 Railgun",
				KeyCombinationString = ParseKeyCombination("DRDULR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 10,
				Name = "FAF-14 SPEAR Launcher",
				KeyCombinationString = ParseKeyCombination("DDUDD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 11,
				Name = "GL-21 Grenade Launcher",
				KeyCombinationString = ParseKeyCombination("DLULD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 12,
				Name = "LAS-98 Laser Cannon",
				KeyCombinationString = ParseKeyCombination("DLDUL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 13,
				Name = "ARC-3 Arc Thrower",
				KeyCombinationString = ParseKeyCombination("DRDULL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 14,
				Name = "LAS-99 Quasar Cannon",
				KeyCombinationString = ParseKeyCombination("DDULR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 15,
				Name = "B-1 Supply Pack",
				KeyCombinationString = ParseKeyCombination("DLDUUD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 16,
				Name = "SH-32 Shield Generator Pack",
				KeyCombinationString = ParseKeyCombination("DULRLR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 17,
				Name = "AX/AR-23 Guard Dog",
				KeyCombinationString = ParseKeyCombination("DULURD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 18,
				Name = "AX/LAS-5 Guard Dog Rover",
				KeyCombinationString = ParseKeyCombination("DULURR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 19,
				Name = "SH-20 Ballistic Shield Backpack",
				KeyCombinationString = ParseKeyCombination("DLDDUL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 20,
				Name = "LIFT-850 Jump Pack",
				KeyCombinationString = ParseKeyCombination("DUUDU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 21,
				Name = "Orbital Gatling Barrage",
				KeyCombinationString = ParseKeyCombination("RDLUU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 22,
				Name = "Orbital Airburst Strike",
				KeyCombinationString = ParseKeyCombination("RRR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 23,
				Name = "Orbital 120MM HE Barragee",
				KeyCombinationString = ParseKeyCombination("RRDLRD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 24,
				Name = "Orbital 380MM HE Barrage",
				KeyCombinationString = ParseKeyCombination("RDUULDD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 25,
				Name = "Orbital Walking Barrage",
				KeyCombinationString = ParseKeyCombination("RDRDRD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 26,
				Name = "Orbital Laser",
				KeyCombinationString = ParseKeyCombination("RDURD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 27,
				Name = "Orbital Railcannon Strike",
				KeyCombinationString = ParseKeyCombination("RUDDR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 28,
				Name = "Orbital Precision Strike",
				KeyCombinationString = ParseKeyCombination("RRU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 29,
				Name = "Orbital Gas Strike",
				KeyCombinationString = ParseKeyCombination("RRDR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 30,
				Name = "Orbital EMS Strike",
				KeyCombinationString = ParseKeyCombination("RRLD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 31,
				Name = "Orbital Smoke Strike",
				KeyCombinationString = ParseKeyCombination("RRDU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 32,
				Name = "Eagle Strafing Run",
				KeyCombinationString = ParseKeyCombination("URR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 33,
				Name = "Eagle Airstrike",
				KeyCombinationString = ParseKeyCombination("URDR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 34,
				Name = "Eagle Cluster Bomb",
				KeyCombinationString = ParseKeyCombination("URDDR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 35,
				Name = "Eagle Napalm Airstrike",
				KeyCombinationString = ParseKeyCombination("URDU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 36,
				Name = "Eagle Smoke Strike",
				KeyCombinationString = ParseKeyCombination("ULUD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 37,
				Name = "Eagle 110MM Rocket Pods",
				KeyCombinationString = ParseKeyCombination("URUL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 38,
				Name = "Eagle 500kg Bomb",
				KeyCombinationString = ParseKeyCombination("URDDD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 39,
				Name = "MD-6 Anti-Personnel Minefield",
				KeyCombinationString = ParseKeyCombination("DLUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 40,
				Name = "MD-I4 Incendiary Mines",
				KeyCombinationString = ParseKeyCombination("DLLD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 41,
				Name = "FX-12 Shield Generator Relay",
				KeyCombinationString = ParseKeyCombination("DDLRLR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 42,
				Name = "A/ARC-3 Tesla Tower",
				KeyCombinationString = ParseKeyCombination("DURULR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 43,
				Name = "A/G-16 Gatling Sentry",
				KeyCombinationString = ParseKeyCombination("DURL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 44,
				Name = "A/M-12 Mortar Sentry",
				KeyCombinationString = ParseKeyCombination("DURRD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 45,
				Name = "A/MG-43 Machine Gun Sentry",
				KeyCombinationString = ParseKeyCombination("DURRU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 46,
				Name = "A/AC-8 Autocannon Sentry",
				KeyCombinationString = ParseKeyCombination("DURULU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 47,
				Name = "A/MLS-4X Rocket Sentry",
				KeyCombinationString = ParseKeyCombination("DURL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 48,
				Name = "A/M-23 EMS Mortar Sentry",
				KeyCombinationString = ParseKeyCombination("DURL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 49,
				Name = "E/MG-101 HMG Emplacement",
				KeyCombinationString = ParseKeyCombination("DULRRL")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 50,
				Name = "Resupply",
				KeyCombinationString = ParseKeyCombination("DDUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 51,
				Name = "Reinforce",
				KeyCombinationString = ParseKeyCombination("UDRLU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 52,
				Name = "SOS Beacon",
				KeyCombinationString = ParseKeyCombination("UDRU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 53,
				Name = "SSSD Delivery",
				KeyCombinationString = ParseKeyCombination("DDDUU")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 54,
				Name = "Eagle Rearm",
				KeyCombinationString = ParseKeyCombination("UULUR")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 55,
				Name = "Seismic Probe",
				KeyCombinationString = ParseKeyCombination("UULRDD")
			});
			hellKeyMaps.Add(new HellKeyMapping
			{
				ID = 56,
				Name = "NUX-223 Hellbomb",
				KeyCombinationString = ParseKeyCombination("DULDURDU")
			});
		}

		public HellKeyMapping GetHellKeyMappingById(string idString)
		{
			int id;

			// Try parsing the input string to an integer
			if (int.TryParse(idString, out id))
			{
				// Search the list for the item with matching ID
				return hellKeyMaps.FirstOrDefault(mapping => mapping.ID == id);
			}
			else
			{
				// Handle invalid input (e.g., not a valid integer)
				Console.WriteLine("Invalid ID format: " + idString);
				return null; // Or return a default value if applicable
			}
		}

		private string ParseKeyCombination(string simplifiedSyntax)
		{
			StringBuilder fullCommand = new StringBuilder();

			foreach (char action in simplifiedSyntax)
			{
				switch (action)
				{
					case 'U':
						fullCommand.Append($"_UP_{Sleep()}"); // GetRandomSleep() returns a random sleep value
						break;
					case 'D':
						fullCommand.Append($"_DOWN_{Sleep()}");
						break;
					case 'L':
						fullCommand.Append($"_LEFT_{Sleep()}");
						break;
					case 'R':
						fullCommand.Append($"_RIGHT_{Sleep()}");
						break;
					default:
						// Handle unrecognized action
						break;
				}
			}

			return $"&LCTRL_{Sleep()}" + fullCommand.ToString() + "_^LCTRL"; // Add start and end of the command line
		}

		private void button9_Click(object sender, EventArgs e)
		{
			if (activated)
			{
				btnSave.Enabled = true;
				button1.Enabled = true;
				btnNum0.Enabled = true;
				btnNum1.Enabled = true;
				btnNum2.Enabled = true;
				btnNum3.Enabled = true;
				btnNum4.Enabled = true;
				btnNum5.Enabled = true;
				btnNum6.Enabled = true;
				btnNum7.Enabled = true;
				btnNum8.Enabled = true;
				btnNum9.Enabled = true;

				// Todo start VirtualKeyboard
				button9.Text = "Activate";
				activated = false;
			}
			else
			{
				btnSave.Enabled = false;
				button1.Enabled = false;
				btnNum0.Enabled = false;
				btnNum1.Enabled = false;
				btnNum2.Enabled = false;
				btnNum3.Enabled = false;
				btnNum4.Enabled = false;
				btnNum5.Enabled = false;
				btnNum6.Enabled = false;
				btnNum7.Enabled = false;
				btnNum8.Enabled = false;
				btnNum9.Enabled = false;

				DisableRightButtons();

				// Todo start VirtualKeyboard
				button9.Text = "Deactivate";
				activated = true;
			}

		}
	}

	public class HellKeyMapping
	{
		public int ID { get; set; }
		public string Name { get; set; }
		public string KeyCombinationString { get; set; }
		//public Image Image { get; set; }
		public HellKeyMapping()
		{

		}
	}
}
