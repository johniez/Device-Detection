﻿/* *********************************************************************
 * This Source Code Form is copyright of 51Degrees Mobile Experts Limited. 
 * Copyright 2015 51Degrees Mobile Experts Limited, 5 Charlotte Close,
 * Caversham, Reading, Berkshire, United Kingdom RG4 7BY
 * 
 * This Source Code Form is the subject of the following patent 
 * applications, owned by 51Degrees Mobile Experts Limited of 5 Charlotte
 * Close, Caversham, Reading, Berkshire, United Kingdom RG4 7BY: 
 * European Patent Application No. 13192291.6; and
 * United States Patent Application Nos. 14/085,223 and 14/085,301.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.
 * 
 * If a copy of the MPL was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 * 
 * This Source Code Form is “Incompatible With Secondary Licenses”, as
 * defined by the Mozilla Public License, v. 2.0.
 * ********************************************************************* */

using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using FiftyOne.UnitTests;
using System.Collections.Specialized;

namespace UnitTests.API.Lite
{
    [TestClass]
    public class Pattern : PatternBase
    {
        [TestInitialize]
        public void Initialise()
        {
            if (_wrapper == null) { _wrapper = CreateWrapper(); }
        }

        [TestCleanup]
        public void CleanUp()
        {
            Dispose();
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_NullUserAgent()
        {
            using (var result = _wrapper.Match((string)null))
            {
                Console.WriteLine(result.ToString());
            }
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_EmptyUserAgent()
        {
            using (var result = _wrapper.Match(String.Empty))
            {
                Console.WriteLine(result.ToString());
            }
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_LongUserAgent()
        {
            var userAgent = String.Join(" ", UserAgentGenerator.GetEnumerable(10, 10));
            using (var result = _wrapper.Match(userAgent))
            {
                Console.WriteLine(result.ToString());
            }
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_HttpHeaders()
        {
            var headers = new NameValueCollection();
            foreach (var header in _wrapper.HttpHeaders)
            {
                headers.Add(header, UserAgentGenerator.GetRandomUserAgent(0));
            }
            using (var result = _wrapper.Match(headers))
            {
                foreach (var property in _wrapper.AvailableProperties)
                {
                    Console.WriteLine("{0}: {1}", property, result[property]);
                }
            }
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_DeviceId()
        {
            using (var userAgentMatch = _wrapper.Match(UserAgentGenerator.GetRandomUserAgent(0)))
            {
                using (var deviceIdMatch = _wrapper.MatchForDeviceId(userAgentMatch.DeviceId))
                {
                    Assert.IsTrue(userAgentMatch.DeviceId.Equals(deviceIdMatch.DeviceId));
                    foreach(var propertyName in _wrapper.AvailableProperties)
                    {
                        Assert.IsTrue(userAgentMatch[propertyName].Equals(deviceIdMatch[propertyName]));
                    }
                }
            }
        }

        [TestMethod]
        [TestCategory("API"), TestCategory("Lite")]
        public void LitePatternAPI_FindProfiles()
        {
            string[] properties = new string[3] {"IsMobile", "BrowserName", "PlatformName"};
            string[,] values = new string [3,2] { { "True", "False" }, { "Firefox", "Chrome" }, { "Android", "Windows" } };
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    Console.WriteLine("Testing " + properties[i] + " " + values[i, j]);
                    var profiles = _wrapper.FindProfiles(properties[i], values[i, j]);
                    for (int k = 0; k < profiles.getCount(); k++)
                    {
                        Assert.IsNotNull(profiles.getProfileId(k));
                        Assert.IsTrue(profiles.getProfileIndex(k) >= 0);
                        Assert.IsTrue(profiles.getProfileId(k) >= 0);
                    }
                }
            }
        }

        protected override string DataFile
        {
            get { return Constants.LITE_PATTERN_V32; }
        }
    }
}
